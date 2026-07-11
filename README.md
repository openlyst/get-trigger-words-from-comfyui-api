# gtwc - Get Trigger Words from ComfyUI

A CLI tool written in C that lists LoRAs from a ComfyUI server and extracts their trigger words.

## Dependencies

- **libcurl** - HTTP requests
- **json-c** - JSON parsing
- gcc / make

On Arch:

```
pacman -S curl json-c
```

On Debian/Ubuntu:

```
apt install libcurl4-openssl-dev libjson-c-dev
```

## Building

```
make
```

For a debug build:

```
make CFLAGS="-Wall -Wextra -g -O0 $(pkg-config --cflags json-c libcurl)"
```

## Installation

```
sudo make install
```

This puts the binary at `/usr/local/bin/gtwc`. To uninstall:

```
sudo make uninstall
```

## Usage

### Set the server URL

```
./gtwc set url http://127.0.0.1:8188
```

This is stored in `~/.config/gtwc/config`. Defaults to `http://127.0.0.1:8188` if not set.

### Set an API key (optional)

Only needed if your ComfyUI server requires authentication:

```
./gtwc set apikey my-secret-key
./gtwc set apikey clear    # remove stored key
```

### List all LoRAs

```
./gtwc list
```

Output:

```
Found 51 LoRA(s):
  [0] Bambi_il.safetensors
  [1] Bluey_IL.safetensors
  [2] BoykisserSDXL-V2.safetensors
  ...
```

### Get trigger words for a LoRA

Accepts a partial name match or an index from `list`:

```
./gtwc get Bambi
./gtwc get 0
./gtwc get "bolt the dog"
```

Output:

```
LoRA: Bambi_il.safetensors
Trigger words:
  Bambi  (dataset name, count: 20)
```

Each trigger word is annotated with its source:

- **explicit** - found in a dedicated trigger word metadata field
- **dataset name** - derived from the training dataset folder name
- **tag frequency** - high-frequency tag from `ss_tag_frequency`
- **filename** - fallback derived from the LoRA filename

## How it works

The tool talks to two ComfyUI API endpoints:

1. `/object_info/LoraLoader` - returns the list of available LoRA files
2. `/view_metadata/loras?filename=<name>` - returns the safetensors `__metadata__` dict

Trigger words are extracted from the metadata using these strategies, in priority order:

1. **Explicit fields** - checks `civitai_trigger_words`, `trigger_word`, `trigger_words`, `ss_trigger_word`, and `civitai` (object form)
2. **Dataset name** - parses `ss_tag_frequency` keys and strips numeric prefixes (e.g. `100_Bambi` -> `Bambi`). Generic folder names like `img`, `data`, `nueva carpeta` are skipped.
3. **Tag frequency** - parses `ss_tag_frequency` and `ss_dataset_dirs`. Tags that appear in >= 70% of training images (based on `img_count`) are considered trigger words. Common booru tags (solo, 1girl, smile, furry, etc.) are filtered out. For caption-style training data (long natural language captions), the most common leading token is extracted instead.
4. **Filename fallback** - if no training metadata exists, a trigger word is derived from the LoRA filename with version suffixes stripped.

## Config file

Stored at `~/.config/gtwc/config` in plain `key=value` format:

```
url=http://127.0.0.1:8188
apikey=optional-key-here
```

## License

AGPL-3.0
