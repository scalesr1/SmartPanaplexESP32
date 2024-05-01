#include "EEPROMConfig.h"
#include "web.h"
#include <regex>

extern EEPROMConfig config;
extern const char* endMarker;

const char* getTemplateString(const char* key, const char* name2html[][2]) {
  // n^2 but hey ho
  for (int i=0; strcmp(endMarker, name2html[i][0]) != 0; i++) {
    if (strcmp(name2html[i][0], key) == 0) {
      return name2html[i][1];
    }
  }

  return endMarker;
}

/**
 * Render one item
 */
void render(AsyncResponseStream *response, BaseConfigItem* item, const char* htmlTemplate) {
  // The HTML element template we are trying to render
  String html = htmlTemplate;

  // Get the item value as a string (as everything in HTML is a string)
  String itemValue = item->toString();

  // This is for HTML controls that use the 'selected' property
  // We want to replace %selected==some value% with 'selected' if itemValue is 'some value'
  String selected = "%selected==" + itemValue + "%";
  html.replace(selected, "selected");

  // Same, but for HTML controls that use the 'checked' property. Sigh.
  selected = "%checked==" + itemValue + "%";
  html.replace(selected, "checked");

  // Now replace '{v}' in the template with itemValue, if '{v}' is in the template
  html.replace("{v}", itemValue);

  // Now we want to get rid of all the %selected==some other value% strings
  std::regex pattern("%selected==[^%]+%");
  std::string result = std::regex_replace(html.c_str(), pattern, "");

  // Same for checked
  pattern = "%checked==[^%]+%";
  result = std::regex_replace(result, pattern, "");

  // Finally send this string to the browser
  response->printf(result.c_str());
}

/**
 * Render the whole root page
 */
void handleRoot(AsyncWebServerRequest *request, BaseConfigItem* rootConfig, const char* name2html[][2]) {
  Serial.println("Handling /");
	AsyncResponseStream *response = request->beginResponseStream("text/html");

  // Send everything up to the body
	response->print(R_SCALES_WEB_HTTP_HEAD);
	response->print(R_SCALES_WEB_HTTP_SCRIPT);
	response->print(R_SCALES_WEB_HTTP_STYLE);
	response->print(R_SCALES_WEB_HTTP_HEAD_END);

  // Send anything we want at the top of the page
	response->print("<h1>");
	response->print("SmartSocket 12 digit Clock");
	response->print("</h1>");

  // Send the form that will end up POSTing to /save
  response->print(R_SCALES_WEB_HTTP_FORM_START);

  // Send all the elements in the form
  for (int i=0; strcmp(endMarker, name2html[i][0]) != 0; i++) {
    BaseConfigItem* item = rootConfig->get(name2html[i][0]);
    if (item != 0) {
      render(response, item, name2html[i][1]);
    }
  }

  // Done with the form
  response->print(R_SCALES_WEB_HTTP_FORM_END);

  // We could send some more HTML that we want at the end of the page here

  response->print("<br>");

  response->print("- 2024 - https://www.panaplex.co.uk");

  // Finish up the web page
	response->print(R_SCALES_WEB_HTTP_END);

  // Send it all to the browser
	request->send(response);
}

/**
 * The root page does a POST to http://<ip address>/save. Extract all the form
 * values and send a re-direct to a page that just says 'saved...'
 */

void handleSave(AsyncWebServerRequest *request, BaseConfigItem* rootConfig) {
  // Loop through all the config items
  rootConfig->forEach([request](BaseConfigItem& item) {

    // If the POST from the browser has a value for this config item...
    if(request->hasParam(item.name, true)) {
      // Retrieve value from POST...
      const char* value = request->getParam(item.name, true)->value().c_str();
      Serial.printf("POST[%s]: %s\n", item.name, value);

      // And set the config item value
      item.fromString(value);
    } else {
      // Sigh. If an HTML checkbox is not checked, the POST won't contain any value for it,
      // so we assume that it was unchecked and we further assume that the config item
      // is a boolean that should be set to false
      item.fromString("false");
    }

    // Mark the config item to be saved to EEPROM
    item.put();
  });

  // Save all the config items to EEPROM
  config.commit();

  // Display the 'Saved...' message for a short time
  request->redirect("/saved");
}


/**
 * Display 'saved...' for 3 seconds then redirect back to root
 */
void handleSaved(AsyncWebServerRequest *request, BaseConfigItem* rootConfig) {
	AsyncResponseStream *response = request->beginResponseStream("text/html");

  // This R"(...)" syntax is a string literal. Anything between those brackets is used verbatim, newlines and all
	response->print(R"(
    <html lang="en">
    <head>
      <meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no"/>
      <meta http-equiv="refresh" content="3; url=/" />
      <title>Panaplex Clock</title>
    </head>
    <body>
    <p style="text-align: center;;font-family:verdana;font-size:1em;">Saved...</p>
    </body>
    </html>
    )"
  );

	request->send(response);
}

/**
 * Tell the web server how to handle URLs
 */
void createWebPages(AsyncWebServer& server, BaseConfigItem* configs, const char* name2html[][2])
{
	server.on("/", HTTP_GET, [configs, name2html](AsyncWebServerRequest *request) { handleRoot(request, configs, name2html); }).setFilter(ON_STA_FILTER);
	server.on("/save", HTTP_POST, [configs](AsyncWebServerRequest *request) { handleSave(request, configs); }).setFilter(ON_STA_FILTER);
	server.on("/saved", HTTP_GET, [configs](AsyncWebServerRequest *request) { handleSaved(request, configs); }).setFilter(ON_STA_FILTER);

  server.begin();
}
