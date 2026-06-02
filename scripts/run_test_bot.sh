#!/usr/bin/env bash
set -euo pipefail

# Load .env file if present (not committed)
if [ -f .env ]; then
  set -a
  # shellcheck source=/dev/null
  . .env
  set +a
fi

TOKEN="${CPPGRAM_BOT_TOKEN:-}"

if [ -z "$TOKEN" ]; then
  cat <<EOF
CPPGRAM_BOT_TOKEN not set.
Create a .env file with: CPPGRAM_BOT_TOKEN=your_token_here
Or export the variable in your shell:
  export CPPGRAM_BOT_TOKEN="your_token_here"
EOF
  exit 1
fi

exec build/examples/test_bot "$@"
