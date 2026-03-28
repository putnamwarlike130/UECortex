"""
fab_tools.py — Fab marketplace tools for UECortex
Registers fab_search and fab_get_asset as dynamic MCP tools.

Usage (from UE Python console or via python_exec_file):
    exec(open('Scripts/fab_tools.py').read())
Or via MCP:
    python_exec_file with path "Scripts/fab_tools.py"
"""

import urllib.request
import urllib.parse
import json
import sys

# ── Shared helpers ────────────────────────────────────────────────────────────

FAB_BASE = "https://www.fab.com"
HEADERS = {
    "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36",
    "Accept": "application/json",
    "X-Requested-With": "XMLHttpRequest",
}

def fab_request(url):
    req = urllib.request.Request(url, headers=HEADERS)
    with urllib.request.urlopen(req, timeout=15) as resp:
        return json.loads(resp.read().decode("utf-8"))

def format_price(price_info):
    if not price_info:
        return "Unknown"
    if price_info.get("discountPrice") == 0 or price_info.get("basePrice") == 0:
        return "Free"
    amount = price_info.get("discountPrice") or price_info.get("basePrice")
    currency = price_info.get("currencyCode", "USD")
    if amount:
        return f"{currency} {amount / 100:.2f}"
    return "Unknown"

# ── Tool: fab_search ──────────────────────────────────────────────────────────

FAB_SEARCH_CODE = r"""
import urllib.request, urllib.parse, json, sys

FAB_BASE = "https://www.fab.com"
HEADERS = {
    "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64)",
    "Accept": "application/json",
    "X-Requested-With": "XMLHttpRequest",
}

query     = _mcp_params.get("query", "")
count     = int(_mcp_params.get("count", 10))
free_only = _mcp_params.get("free_only", False)
sort_by   = _mcp_params.get("sort_by", "relevance")  # relevance | price_asc | price_desc | rating | newest
ue_ver    = _mcp_params.get("ue_version", "")         # e.g. "5.5"

params = {
    "q":        query,
    "channels": "unreal-engine",
    "count":    min(count, 50),
    "sort_by":  sort_by,
}
if free_only:
    params["is_free"] = "true"
if ue_ver:
    params["engine_version_compatibility"] = ue_ver

url = FAB_BASE + "/i/listings/search?" + urllib.parse.urlencode(params)
req = urllib.request.Request(url, headers=HEADERS)

try:
    with urllib.request.urlopen(req, timeout=15) as resp:
        data = json.loads(resp.read().decode("utf-8"))
except Exception as e:
    _mcp_result = f"ERROR: {e}"
    sys.exit()

results = data.get("results", [])
output = []
for r in results:
    sp = r.get("startingPrice") or {}
    price = "Free" if r.get("isFree") else (f"USD {sp['price']:.2f}" if sp.get("price") else "Unknown")

    output.append({
        "title":   r.get("title", ""),
        "uid":     r.get("uid", ""),
        "seller":  (r.get("user") or {}).get("sellerName", ""),
        "price":   price,
        "rating":  r.get("averageRating", "N/A"),
        "reviews": r.get("reviewCount", 0),
        "url":     f"https://www.fab.com/listings/{r.get('uid', '')}",
    })

total = data.get("count", len(output))
_mcp_result = f"Found {total} results (showing {len(output)})\n" + json.dumps(output, indent=2)
"""

# ── Tool: fab_get_asset ───────────────────────────────────────────────────────

FAB_GET_ASSET_CODE = r"""
import urllib.request, json, sys

FAB_BASE = "https://www.fab.com"
HEADERS = {
    "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64)",
    "Accept": "application/json",
    "X-Requested-With": "XMLHttpRequest",
}

uid = _mcp_params.get("uid", "").strip()
if not uid:
    _mcp_result = "ERROR: 'uid' param required. Get it from fab_search results."
    sys.exit()

url = FAB_BASE + f"/i/listings/{uid}"
req = urllib.request.Request(url, headers=HEADERS)

try:
    with urllib.request.urlopen(req, timeout=15) as resp:
        d = json.loads(resp.read().decode("utf-8"))
except Exception as e:
    _mcp_result = f"ERROR: {e}"
    sys.exit()

sp = d.get("startingPrice") or {}
price = "Free" if d.get("isFree") else (f"USD {sp['price']:.2f}" if sp.get("price") else "Unknown")

formats = [(f.get("assetFormatType") or {}).get("name", "") for f in d.get("assetFormats", [])]
ue_versions = []
for fmt in d.get("assetFormats", []):
    ue_versions += (fmt.get("technicalSpecs") or {}).get("unrealEngineEngineVersions") or []
tags = [t.get("name", "") for t in (d.get("tags") or [])[:15]]
category = (d.get("category") or {}).get("name", "")

result = {
    "title":       d.get("title"),
    "uid":         d.get("uid"),
    "seller":      (d.get("user") or {}).get("sellerName"),
    "description": (d.get("description") or "")[:800],
    "price":       price,
    "rating":      d.get("averageRating"),
    "reviews":     d.get("reviewCount"),
    "category":    category,
    "tags":        tags,
    "formats":     formats,
    "ue_versions": ue_versions,
    "url":         f"https://www.fab.com/listings/{uid}",
    "thumbnail":   (d.get("thumbnails") or [{}])[0].get("mediaUrl") or (d.get("thumbnails") or [{}])[0].get("url", ""),
}

_mcp_result = json.dumps(result, indent=2)
"""

# ── Registration ─────────────────────────────────────────────────────────────
# Register each tool separately via the MCP python_register_tool tool.
# Do NOT self-register by calling localhost from within python_exec_file —
# that re-enters the MCP server from the game thread and deadlocks on the
# second call. Use the MCP tool directly instead:
#
#   python_register_tool { name: "fab_search",    description: ..., code: FAB_SEARCH_CODE }
#   python_register_tool { name: "fab_get_asset", description: ..., code: FAB_GET_ASSET_CODE }
#
# This file is kept as the source-of-truth for the tool code strings above.
