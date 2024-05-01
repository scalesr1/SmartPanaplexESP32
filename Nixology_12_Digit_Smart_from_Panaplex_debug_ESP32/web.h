#ifndef R_SCALES_WEB_H
#define R_SCALES_WEB_H

#include <map>
#include <string>
#include <ESPAsyncWebServer.h>
#include "ConfigItem.h"

void createWebPages(AsyncWebServer& server, BaseConfigItem* configs, const char* name2html[][2]);

const char R_SCALES_WEB_HTTP_HEAD[]	=
R"(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no"/>
    <title>Panaplex Clock</title>
)";

const char R_SCALES_WEB_HTTP_STYLE[] =
R"(
    <style>
      .c {text-align: center;}
      .checkbox_div { padding: 10px 0px;}
      div {display: block; padding:0px 0px 5px 0px;font-size:1em;}
      input {width:95%;padding:5px;font-size:1em;}
      input[type="checkbox"] {width:auto;padding:5px;font-size:1em;}
      select {width:95%;padding:5px;font-size:1em;}
      body {text-align: center;font-family:verdana;}
      button {border:0;border-radius:0.3rem;background-color:#1fa3ec;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;}
      button:active {border:0;border-radius:0.3rem;background-color:#1f1f1f;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;}
    </style>
)";

const char R_SCALES_WEB_HTTP_SCRIPT[] =
R"(
    <script>
      function c(l) {
        document.getElementById('s').value=l.innerText||l.textContent;
        document.getElementById('p').focus();
      };
      function t() {
        var x=document.getElementById('p');
        if(x.type === 'password') {
          x.type='text';
        } else {
          x.type='password';
        }
      }
    </script>
)";

const char R_SCALES_WEB_HTTP_HEAD_END[] =
R"(
  </head>
  <body>
    <div style='text-align:left;display:inline-block;min-width:260px;'>
)";

const char R_SCALES_WEB_HTTP_FORM_START[] =
R"(
    <form method='post' action='/save'>
)";

const char R_SCALES_WEB_HTTP_FORM_PARAM[] =
R"(
    <br/><label for='{i}'>{L}</label><input type='{t}' id='{i}' name='{n}' length={l} placeholder='{p}' value='{v}' {c}>
)";

const char R_SCALES_WEB_HTTP_FORM_END[] =
R"(
    <br/>
    <button type='submit'>save</button>
    </form>
)";

const char R_SCALES_WEB_HTTP_END[] = 
R"(
    </div>
  </body>
</html>
)";

#endif