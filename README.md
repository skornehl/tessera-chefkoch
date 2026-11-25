# Chefkoch

> **Work in progress (WIP).** This plugin is in its first versions and runs on one kitchen screen so far. Expect
> changes, and please report what does not work.

Cook along on a [Tessera](https://github.com/MaxGramser/homeassistant_espscreen) screen: the current step of the
recipe that [ha-chefkoch](https://github.com/skornehl/ha-chefkoch) is cooking, large enough to read from the stove.

- On top: the recipe's name and where you are ("Step 3 of 7", or "Ingredients" before the first step).
- In the middle: the step's words in the largest of the screen's fonts that fits the tile.
- At the bottom: **Back**, **Timer 12 min** (when the step names a time and a timer script is chosen) and **Next**,
  which becomes **Done** on the last step.
- A tap on the words goes on (right half) or back (left half).

## Set up

1. Install [ha-chefkoch](https://github.com/skornehl/ha-chefkoch) in Home Assistant and start a recipe once, so
   `sensor.chefkoch_kochschritt` has a value.
2. Add the template sensor below to Home Assistant (in a package or `configuration.yaml`) and reload template entities.
   It hands the step's words to the screen as a list (see "Why a template sensor").
3. In Tessera, open Plugins, **Add with a link**, and paste `https://github.com/skornehl/tessera-chefkoch`. Add it to
   your kitchen screen; the screen builds once.
4. In Layout, place the **Cooking step** tile from the library's Plugins group, as large as you like (3x2 up to the
   whole page; 5x4 on a 10-inch screen reads well from a few metres).
5. In the inspector, choose `sensor.kochen_tessera` as its entity. The keys default to the buttons of ha-chefkoch;
   choose a script under **Timer script** if you want a timer key.

```yaml
template:
  - sensor:
      - name: Kochen Tessera
        unique_id: kochen_tessera
        icon: mdi:chef-hat
        state: "{{ states('sensor.chefkoch_kochschritt')[:100] }}"
        attributes:
          rezept: "{{ state_attr('sensor.chefkoch_kochschritt', 'rezept') or '' }}"
          schritt: "{{ state_attr('sensor.chefkoch_kochschritt', 'schritt') | int(0) }}"
          schritte_gesamt: "{{ state_attr('sensor.chefkoch_kochschritt', 'schritte_gesamt') | int(0) }}"
          timer_minuten: "{{ state_attr('sensor.chefkoch_kochschritt', 'timer_minuten') | int(0) }}"
          letzter_schritt: "{{ state_attr('sensor.chefkoch_kochschritt', 'letzter_schritt') | bool(false) }}"
          teile: >-
            {% set ns = namespace(w=[]) %}
            {% for zeile in (state_attr('sensor.chefkoch_kochschritt', 'text') or '').split('\n') if zeile | trim %}
              {% if ns.w %}{% set ns.w = ns.w + ['\n'] %}{% endif %}
              {% set ns.w = ns.w + zeile.split() %}
            {% endfor %}
            {{ ns.w }}
```

## Why a template sensor

Tessera sends each text of a tile's data to the screen cut at 48 bytes, so a whole recipe step in one attribute would
end after a few words. A list of words arrives whole (up to about 2.5 KB per tile), and the plugin joins it again;
`"\n"` in the list starts a new line (the ingredients). A later version of ha-chefkoch may give this attribute itself.

## Good to know

- The tile follows the sensor: Back, Next and the timer act in Home Assistant, and the screen shows the new step as
  soon as the sensor changes.
- Actions it calls: `button.press` (back, next, done) and `script.turn_on` (timer). The screen must be allowed to
  perform actions in Home Assistant.
- No internet service: everything comes from Home Assistant.

## Releases

Every release has a zip of the plugin folder attached (`tessera-chefkoch-v<version>.zip`), ready to unpack into `/config/tessera-plugins/chefkoch/`.
