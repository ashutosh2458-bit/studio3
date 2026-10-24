#!/usr/bin/env bash
set -e
emcc src/main.cpp -O2 -std=c++17 --no-entry -o web/designer.js \
  -sEXPORTED_FUNCTIONS=_render_svg,_export_js,_describe,_ai_learn,_ai_suggest,_ai_report \
  -sEXPORTED_RUNTIME_METHODS=ccall -sENVIRONMENT=web -sALLOW_MEMORY_GROWTH=1
echo "Built web/designer.js + web/designer.wasm"
