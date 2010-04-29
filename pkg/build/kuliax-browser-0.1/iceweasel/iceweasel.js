// This is the Debian specific preferences file for Iceweasel
// You can make any change in here, it is the purpose of this file.
// You can, with this file and all files present in the
// /etc/iceweasel/pref directory, override any preference that is
// present in /usr/lib/iceweasel/defaults/preferences directory.
// While your changes will be kept on upgrade if you modify files in
// /etc/iceweasel/pref, please note that they won't be kept if you
// do make your changes in /usr/lib/iceweasel/defaults/preferences.
//
// Note that lockPref is allowed in these preferences files if you
// don't want users to be able to override some preferences.

pref("extensions.update.enabled", true);

// Use LANG environment variable to choose locale
pref("intl.locale.matchOS", true);

// Disable default browser checking.
pref("browser.shell.checkDefaultBrowser", false);

// Prefered fonts
pref("font.default.x-western", "sans-serif");
pref("font.minimum-size.x-western", 12);
pref("font.name.monospace.x-western", "DejaVu Sans Mono");
pref("font.name.sans-serif.x-western", "DejaVu Sans");
pref("font.name.serif.x-western", "DejaVu Serif");
pref("font.size.variable.x-western", 15);

// Welcome URL
pref("startup.homepage_welcome_url","file:///usr/share/kuliax-releasenotes/index.html");
