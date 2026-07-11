# gtwc - Get Trigger Words from ComfyUI

A CLI tool to list LoRAs and extract trigger words from a ComfyUI server.

## Building

Requires `libcurl` and `json-c`:

```
make
```

## Usage

```
./gtwc set url http://127.0.0.1:8188
./gtwc set apikey <key>       # optional, only if your server requires auth
./gtwc list
./gtwc get "Bambi"
./gtwc get 0                  # by index from list
```

## How it works

The tool queries the ComfyUI API (`/object_info/LoraLoader` for the lora list,
`/view_metadata/loras` for safetensors metadata) and extracts trigger words
from the `ss_tag_frequency` training metadata. It uses several strategies in
priority order:

1. **Explicit fields** - `civitai_trigger_words`, `trigger_word`, etc.
2. **Dataset name** - the training folder name (e.g. `100_Bambi` -> `Bambi`)
3. **Tag frequency** - high-frequency tags from `ss_tag_frequency`, filtered
   against common booru tags and thresholded by image count
4. **Filename fallback** - derives a trigger word from the lora filename when
   no training metadata is available

## License

AGPL-3.0
