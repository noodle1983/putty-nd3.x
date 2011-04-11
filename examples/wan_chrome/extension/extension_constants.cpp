
#include "extension_constants.h"

const char* kDescription = "description";
const char* kName = "name";
const char* kTheme = "theme";
const char* kThemeColors = "colors";
const char* kThemeDisplayProperties = "properties";
const char* kThemeImages = "images";
const char* kThemeTints = "tints";
const char* kVersion = "version";

const char* kInvalidDescription =
"Invalid value for 'description'.";
const char* kInvalidName =
"Required value 'name' is missing or invalid.";
const char* kInvalidTheme =
"Invalid value for 'theme'.";
const char* kInvalidThemeColors =
"Invalid value for theme colors - colors must be integers";
const char* kInvalidThemeImages =
"Invalid value for theme images - images must be strings.";
const char* kInvalidThemeImagesMissing =
"An image specified in the theme is missing.";
const char* kInvalidThemeTints =
"Invalid value for theme images - tints must be decimal numbers.";
const char* kInvalidVersion =
"Required value 'version' is missing or invalid. It must be between 1-4 "
"dot-separated integers each between 0 and 65536.";