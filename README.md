# Infinite Design Studio

C++ -> WebAssembly generative design app. Each design is a function of a seed, can be exported as
JavaScript (canvas) or SVG, and a small on-device AI learns which designs you like.

## Deploy on GitHub
1. Create a repo and push this folder to the `main` branch.
2. In the repo: Settings -> Pages -> Source: **GitHub Actions**.
3. The workflow builds the C++ with Emscripten and publishes `web/`. Your site appears at
   `https://<user>.github.io/<repo>/`.

## Run locally
```
source /path/to/emsdk/emsdk_env.sh
bash build.sh
python3 -m http.server -d web 8000
```
Then open http://localhost:8000.

## Extend it
Add a new curve in `pt()` and `JS_POINT[]` in `src/main.cpp`, raise the style range in `make()`, and add a slot in `feat()`.
