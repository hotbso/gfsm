# Global Forests Seasons Manager

"Global Forests v2" for XP12 on top of orthophotos created with Ortho4XP 1.30 doesn't show correct seasons as the season information is not encoded in the orthophotos.

This plugin switches GF's seasons according to the date used in flight initialization. It uses the astronomical seasons.

The plugin creates a menu entry where you can enable or disable "Orthophoto mode". When changing the mode you should restart your flight. The mode setting is persistent across program starts.

If the plugin is removed or "Orthophoto mode" is disabled the scenery behaves in "XP12's native mode" as before.

## Installation
- download the latest release here: https://github.com/hotbso/gfsm/releases
- within GF's scenery folder backup file library.txt and unzip the downloaded file here (and not the "resources/plugins" folder!).

## Deinstallation
- restore the backup version of library.txt
- delete the "plugin" folder within GF's scenery folder (and not the "resources/plugins" folder!)
