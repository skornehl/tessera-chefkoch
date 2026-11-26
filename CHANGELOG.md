# Changelog

## 0.1.2
- Uses less of the memory inside the chip, which the screen shares with Wi-Fi: the step's words are laid out
  straight from where they are kept, without copies of the text.
- Empty lines and a line break at the end of a step no longer count as lines.

## 0.1.1
- Lines (the ingredients) spread over up to three columns, so a long list keeps a large font instead of the
  smallest one.

## 0.1.0
- First version (work in progress): a tile with the current step of the recipe ha-chefkoch cooks, in the largest font
  that fits, with keys for back, the step's timer and next (done on the last step). A tap on the words goes on or back.
- Needs a sensor with the attribute `teile` (the step's words as a list); see the README.
