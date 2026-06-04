#!/bin/bash

BASE_URL="http://127.0.0.1:7878"
PASS=0
FAIL=0

# ── Colours ───────────────────────────────────────────────────────────────────
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

pass()    { echo -e "${GREEN}  [PASS]${NC} $1"; ((PASS++)); }
fail()    { echo -e "${RED}  [FAIL]${NC} $1"; ((FAIL++)); }
section() { echo -e "\n${YELLOW}── $1 ──${NC}"; }

# ── HTTP check via curl ───────────────────────────────────────────────────────
check() {
    local description="$1"
    local expected_status="$2"
    local expected_body="$3"
    shift 3
    local curl_args=("$@")

    local response
    local actual_status
    local actual_body

    response=$(curl -s -w "\n__STATUS__%{http_code}" "${curl_args[@]}")
    actual_status=$(echo "$response" | grep "__STATUS__" | sed 's/__STATUS__//')
    actual_body=$(echo "$response" | sed '/__STATUS__/d')

    if [ "$actual_status" != "$expected_status" ]; then
        fail "$description - expected HTTP $expected_status, got HTTP $actual_status"
        return
    fi

    if [ -n "$expected_body" ] && ! echo "$actual_body" | grep -q "$expected_body"; then
        fail "$description - expected body to contain '$expected_body', got '$actual_body'"
        return
    fi

    pass "$description"
}

# ── Raw socket check via netcat ───────────────────────────────────────────────
check_raw_400() {
    local description="$1"
    local raw_data="$2"
    local response
    local actual_status

    response=$(echo -e "$raw_data" | nc -q0 -w1 127.0.0.1 7878 2>/dev/null)
    actual_status=$(echo "$response" | grep -oP 'HTTP/1\.[01] \K[0-9]+' | head -1)

    if [ "$actual_status" = "400" ]; then
        pass "$description"
    else
        fail "$description - expected HTTP 400, got HTTP ${actual_status:-no response}"
    fi
}

# ── Tests ─────────────────────────────────────────────────────────────────────

section "Happy Paths"
check "GET  /           → 200 + Hello World"  "200" "Hello, World!"   "$BASE_URL/"
check "GET  /about      → 200 + about text"   "200" "Simple HTTP"     "$BASE_URL/about"
check "POST /echo       → 200 + echo msg"     "200" "Echo:"           -X POST -d "hello" "$BASE_URL/echo"

section "404 Not Found"
check "GET  /nope       → 404"                "404" "404 Not Found"   "$BASE_URL/nope"
check "GET  /about/sub  → 404"                "404" "404 Not Found"   "$BASE_URL/about/sub"
check "POST /nope       → 404"                "404" "404 Not Found"   -X POST "$BASE_URL/nope"

section "405 Method Not Allowed"
check "POST /           → 405"                "405" "405 Method"      -X POST "$BASE_URL/"
check "POST /about      → 405"                "405" "405 Method"      -X POST "$BASE_URL/about"
check "GET  /echo       → 405"                "405" "405 Method"      -X GET  "$BASE_URL/echo"
check "DELETE /         → 405"                "405" "405 Method"      -X DELETE "$BASE_URL/"

section "400 Bad Request"
check_raw_400 "Raw garbage          → 400" "GARBAGE REQUEST HERE"
check_raw_400 "No CRLF              → 400" "GET / HTTP/1.1"
check_raw_400 "Missing version      → 400" "GET /\r\n\r\n"
check_raw_400 "Header without colon → 400" "GET / HTTP/1.1\r\nBadHeader\r\n\r\n"
check_raw_400 "Completely empty     → 400" ""

section "Path Edge Cases"
check "GET /about/      → 404 (trailing slash)" "404" "404" "$BASE_URL/about/"
check "GET //           → 404 (double slash)"   "404" "404" "$BASE_URL//"

section "Header Edge Cases"
check "Custom headers pass through → 200"  "200" "Hello" \
    -H "X-Custom: whatever" \
    -H "Authorization: Bearer token123" \
    "$BASE_URL/"

section "POST /echo with body sizes"
check "POST /echo small body  → 200"  "200" "Echo:" -X POST -d "hi"                              "$BASE_URL/echo"
check "POST /echo larger body → 200"  "200" "Echo:" -X POST -d "$(python3 -c 'print("X"*1000)')" "$BASE_URL/echo"

# ── Summary ───────────────────────────────────────────────────────────────────
echo -e "\n────────────────────────────────────"
TOTAL=$((PASS + FAIL))
echo -e "  Total:  $TOTAL"
echo -e "  ${GREEN}Passed: $PASS${NC}"
if [ "$FAIL" -gt 0 ]; then
    echo -e "  ${RED}Failed: $FAIL${NC}"
else
    echo -e "  ${GREEN}Failed: $FAIL${NC}"
fi
echo -e "────────────────────────────────────"

[ "$FAIL" -eq 0 ]
