// Infinite Design Studio: C++ compiled to WebAssembly.
// Every design is a pure function of a 31-bit seed, so there are ~2 billion designs
// and each one can be regenerated, exported as code, or scored by the tiny AI.
#include <emscripten/emscripten.h>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace {

const double TAU = 6.283185307179586;
const char* STYLES[] = {"rose", "harmonic", "lissajous", "bloom"};

std::string fmt(const char* f, ...) {
  char buf[768];
  va_list ap; va_start(ap, f);
  vsnprintf(buf, sizeof buf, f, ap);
  va_end(ap);
  return buf;
}

struct Rng {  // xorshift64: tiny, fast, deterministic
  uint64_t s;
  explicit Rng(uint32_t seed) : s((uint64_t)seed * 0x9E3779B97F4A7C15ULL + 0x1234567ULL) { next(); next(); }
  uint32_t next() { s ^= s << 13; s ^= s >> 7; s ^= s << 17; return (uint32_t)(s >> 16); }
  double f() { return (next() & 0xFFFFFF) / 16777216.0; }
  int irange(int a, int b) { return a + (int)(f() * (b - a + 1)); }
  double range(double a, double b, double q) { return std::round((a + (b - a) * f()) * q) / q; }
};

struct Design {
  int style, layers, a, b, hue, spread, sat, light;
  double rot, scale, width, phase;
  bool dark;
};

Design make(uint32_t seed) {
  Rng r(seed);
  Design d;
  d.style  = r.irange(0, 3);
  d.layers = r.irange(4, 14);
  d.a      = r.irange(2, 9);
  d.b      = d.style == 0 ? r.irange(1, 4) : r.irange(1, 9);
  if (d.a == d.b) d.a++;
  d.hue    = r.irange(0, 359);
  d.spread = r.irange(20, 200) * (r.f() < 0.5 ? -1 : 1);
  d.sat    = r.irange(45, 90);
  d.light  = r.irange(45, 70);
  d.rot    = r.range(0.02, 0.35, 1000);
  d.scale  = r.range(0.7, 0.95, 1000);
  d.width  = r.range(0.5, 2.5, 100);
  d.phase  = r.range(0, TAU, 1000);
  d.dark   = r.f() < 0.5;
  return d;
}

void pt(const Design& d, double t, double& x, double& y) {
  switch (d.style) {
    case 0: { double r = cos((double)d.a / d.b * t); x = r * cos(t); y = r * sin(t); break; }
    case 1: x = (cos(d.a * t) + 0.5 * cos(d.b * t)) / 1.5;
            y = (sin(d.a * t) + 0.5 * sin(d.b * t)) / 1.5; break;
    case 2: x = sin(d.a * t + d.phase); y = sin(d.b * t); break;
    default: { double r = 0.7 + 0.3 * sin(d.a * t) * cos(d.b * t); x = r * cos(t); y = r * sin(t); }
  }
}

double tmax(const Design& d) { return d.style == 0 ? TAU * d.b : TAU; }
double hueAt(const Design& d, int i) { return std::fmod(d.hue + (double)d.spread * i / d.layers + 720.0, 360.0); }
const char* bg(const Design& d) { return d.dark ? "#10131a" : "#f6f3ee"; }

std::string svg(const Design& d) {
  const int N = 720;
  std::string o = fmt("<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"-1.1 -1.1 2.2 2.2\" width=\"800\" height=\"800\">\n"
                      "<rect x=\"-1.1\" y=\"-1.1\" width=\"2.2\" height=\"2.2\" fill=\"%s\"/>\n", bg(d));
  for (int i = 0; i < d.layers; i++) {
    double ang = d.rot * i, s = d.scale * (1 - 0.8 * i / d.layers), ca = cos(ang), sa = sin(ang);
    o += fmt("<polyline fill=\"none\" stroke=\"hsl(%.0f,%d%%,%d%%)\" stroke-width=\"%.4f\" stroke-linejoin=\"round\" points=\"",
             hueAt(d, i), d.sat, d.light, d.width * 0.006);
    for (int k = 0; k <= N; k++) {
      double x, y; pt(d, tmax(d) * k / N, x, y);
      o += fmt("%.3f,%.3f ", s * (x * ca - y * sa), s * (x * sa + y * ca));
    }
    o += "\"/>\n";
  }
  return o + "</svg>";
}

const char* JS_POINT[] = {
  "const r=Math.cos(A/B*t);return [r*Math.cos(t),r*Math.sin(t)];",
  "return [(Math.cos(A*t)+0.5*Math.cos(B*t))/1.5,(Math.sin(A*t)+0.5*Math.sin(B*t))/1.5];",
  "return [Math.sin(A*t+PH),Math.sin(B*t)];",
  "const r=0.7+0.3*Math.sin(A*t)*Math.cos(B*t);return [r*Math.cos(t),r*Math.sin(t)];"};

std::string js(const Design& d, uint32_t seed) {
  std::string o = fmt("// Design #%u (%s). Paste into a page's <script> tag.\n"
                      "const A=%d,B=%d,PH=%.3f,LAYERS=%d,ROT=%.3f,SCALE=%.3f,WIDTH=%.2f,HUE=%d,SPREAD=%d,SAT=%d,LIGHT=%d;\n",
                      seed, STYLES[d.style], d.a, d.b, d.phase, d.layers, d.rot, d.scale, d.width, d.hue, d.spread, d.sat, d.light);
  o += "const S=800,c=document.body.appendChild(document.createElement('canvas'));\n"
       "c.width=c.height=S;const g=c.getContext('2d');\n";
  o += fmt("g.fillStyle='%s';g.fillRect(0,0,S,S);\n", bg(d));
  o += std::string("const pt=t=>{") + JS_POINT[d.style] + "};\n";
  o += d.style == 0 ? "const TMAX=2*Math.PI*B;\n" : "const TMAX=2*Math.PI;\n";
  o += "for(let i=0;i<LAYERS;i++){\n"
       "  const ang=ROT*i,s=SCALE*(1-0.8*i/LAYERS),ca=Math.cos(ang),sa=Math.sin(ang);\n"
       "  g.strokeStyle=`hsl(${(HUE+SPREAD*i/LAYERS+720)%360},${SAT}%,${LIGHT}%)`;\n"
       "  g.lineWidth=WIDTH*0.006*S/2.2;g.lineJoin='round';g.beginPath();\n"
       "  for(let k=0;k<=720;k++){\n"
       "    const [x,y]=pt(TMAX*k/720);\n"
       "    const px=(s*(x*ca-y*sa)+1.1)*S/2.2,py=(s*(x*sa+y*ca)+1.1)*S/2.2;\n"
       "    k?g.lineTo(px,py):g.moveTo(px,py);\n"
       "  }\n"
       "  g.stroke();\n}\n";
  return o;
}

// ---- The small AI: online logistic regression over design features ----
const int NF = 13;
std::vector<double> W(NF, 0.0);
int nLikes = 0, nDislikes = 0;

std::vector<double> feat(const Design& d) {
  std::vector<double> x(NF, 0.0);
  x[d.style] = 1;
  x[4] = d.layers / 14.0;  x[5] = d.a / 10.0;      x[6] = d.b / 9.0;
  x[7] = std::fabs((double)d.spread) / 200.0;      x[8] = d.rot / 0.35;
  x[9] = d.width / 2.5;    x[10] = d.dark ? 1 : 0; x[11] = d.sat / 90.0;
  x[12] = 1;
  return x;
}

double score(const std::vector<double>& x) {
  double s = 0; for (int i = 0; i < NF; i++) s += W[i] * x[i]; return s;
}

std::string out;  // buffer for strings returned to JS

}  // namespace

extern "C" {

EMSCRIPTEN_KEEPALIVE const char* render_svg(uint32_t seed) { out = svg(make(seed)); return out.c_str(); }
EMSCRIPTEN_KEEPALIVE const char* export_js(uint32_t seed)  { out = js(make(seed), seed); return out.c_str(); }

EMSCRIPTEN_KEEPALIVE const char* describe(uint32_t seed) {
  Design d = make(seed);
  out = fmt("%s, %d layers, %s background", STYLES[d.style], d.layers, d.dark ? "dark" : "light");
  return out.c_str();
}

EMSCRIPTEN_KEEPALIVE void ai_learn(uint32_t seed, int liked) {
  std::vector<double> x = feat(make(seed));
  double p = 1.0 / (1.0 + exp(-score(x)));
  for (int i = 0; i < NF; i++) W[i] += 0.8 * ((liked ? 1.0 : 0.0) - p) * x[i];
  liked ? nLikes++ : nDislikes++;
}

// Samples 400 candidate seeds and returns the one the model likes best (plus a little noise to explore).
EMSCRIPTEN_KEEPALIVE uint32_t ai_suggest(uint32_t counter) {
  Rng r(counter * 7919u + 13u);
  uint32_t best = r.next() & 0x7FFFFFFF; double bestScore = -1e9;
  for (int i = 0; i < 400; i++) {
    uint32_t s = r.next() & 0x7FFFFFFF;
    double sc = score(feat(make(s))) + 0.3 * r.f();
    if (sc > bestScore) { bestScore = sc; best = s; }
  }
  return best;
}

EMSCRIPTEN_KEEPALIVE const char* ai_report() {
  out = fmt("Learned from %d likes and %d skips.", nLikes, nDislikes);
  if (nLikes + nDislikes < 3) { out += " Rate a few designs and I will start suggesting ones you may like."; return out.c_str(); }
  int bi = 0; for (int i = 1; i < 4; i++) if (W[i] > W[bi]) bi = i;
  out += fmt(" Leaning toward %s curves", STYLES[bi]);
  if (W[4] > 0.2) out += ", many layers"; else if (W[4] < -0.2) out += ", few layers";
  if (W[10] > 0.2) out += ", dark backgrounds"; else if (W[10] < -0.2) out += ", light backgrounds";
  out += ".";
  return out.c_str();
}

}
