# Third-party notices

Cyber AHERO itself is under the MIT License (see `LICENSE`). It includes or
ships with the following third-party software, each under its own license.

## In the source tree

### pl_mpeg (`third_party/pl_mpeg.h`)

MPEG-1 video / MP2 audio decoder by Dominic Szablewski,
<https://github.com/phoboslab/pl_mpeg>. MIT License:

> Copyright (c) Dominic Szablewski
>
> Permission is hereby granted, free of charge, to any person obtaining a copy
> of this software and associated documentation files (the "Software"), to
> deal in the Software without restriction, including without limitation the
> rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
> sell copies of the Software, and to permit persons to whom the Software is
> furnished to do so, subject to the following conditions:
>
> The above copyright notice and this permission notice shall be included in
> all copies or substantial portions of the Software.
>
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
> IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
> FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
> AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
> LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
> FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
> IN THE SOFTWARE.

## In the release packages

The release zips ship the official prebuilt SDL DLLs, downloaded at build time
from <https://github.com/libsdl-org> (not stored in this repository). Their
full license texts are included in each zip under `licenses/`.

- **SDL2** (`SDL2.dll`): zlib License, Copyright (C) 1997-2025 Sam Lantinga.
- **SDL2_image** (`SDL2_image.dll`): zlib License, Copyright (C) 1997-2025
  Sam Lantinga.
- **SDL2_ttf** (`SDL2_ttf.dll`): zlib License, Copyright (C) 1997-2025 Sam
  Lantinga. It statically includes:
  - **FreeType**: Portions of this software are copyright © 2023 The FreeType
    Project (<https://freetype.org>). All rights reserved. Used under the
    FreeType License (FTL).
  - **HarfBuzz**: MIT ("Old MIT") License, Copyright © the HarfBuzz authors,
    <https://harfbuzz.github.io>.

## Not included

- **Games and their artwork.** This repository and its releases contain no
  games, box art, preview videos or descriptions of commercial games. The
  `games/Ejemplo` folder is an original placeholder. Game names mentioned in
  the documentation are trademarks of their respective owners.
- **Scraped content.** `tools/scrape.ps1` downloads covers and details from
  IGDB and SteamGridDB, and optionally videos with yt-dlp. That content belongs
  to its owners and is subject to those services' terms. Keep it in your own
  Library; don't commit or redistribute it.
- **Fonts.** The launcher uses the fonts already installed in Windows, or one
  you place in `assets\fonts\` yourself. No font files are distributed.
