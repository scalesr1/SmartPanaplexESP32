#ifndef R_SCALES_CONFIGS_H
#define R_SCALES_CONFIGS_H

#include <map>
#include <string>

#include "ConfigItem.h"

// Could move all of this to the .ino file, I just put it in this file to try to minimize the
// changes to the .ino file.

// Define some 'config items'. These are named variables that can be stored in EEPROM.
// The "name" argument can be different than the variable name, but it makes life easier to keep them the same.
// They can usually be used as a direct replacement for primitive types such as 'boolean' and 'int'.
ByteConfigItem firsthour("firsthour", 0);
ByteConfigItem lasthour("lasthour", 23);
ByteConfigItem timesystem("timesystem", 0); //12 OR 24 HOUR OPERATION - Used to change between 12 and 24 hour time system - "0" is for 24 hour time and "12" is for 12 hour time
BooleanConfigItem leadingzero("leadingzero", true);
StringConfigItem timezone("timezone", 20, "UK");  // String can be at most 20 characters long
#ifdef DATE_FORMAT
ByteConfigItem dateformat("dateformat", 0);  // 0 = ddmmyy, 1 = mmddyy, 2 = yymmdd
#endif
BooleanConfigItem metricUnits("metricUnits", true);
IntConfigItem PressOffset("PressOffset", 0);
FloatConfigItem TempOffset("TempOffset", 0);
IntConfigItem NumberFont("NumberFont",0);
ByteConfigItem clockTuner("clockTuner", 1);
StringConfigItem MsgString1("MsgString1", 70, "VERY SEXY LADY FRIEND");
StringConfigItem MsgString2("MsgString2", 70, "IM NOT LOOKING BACK BUT I WANT TO LOOK AROUND ME NOW");
StringConfigItem MsgString3("MsgString3", 70, "THE MEASURE OF A LIFE IS A MEASURE OF LOVE AND RESPECT");
StringConfigItem MsgString4("MsgString4", 70, "FREEZE THIS MOMENT A LITTLE BIT LONGER");
StringConfigItem MsgString5("MsgString5", 70, "TIME STAND STILL");
StringConfigItem MsgString6("MsgString6", 70, "IN THE FULLNESS OF TIME");
StringConfigItem MsgString7("MsgString7", 70, "REAL TIME");
StringConfigItem MsgString8("MsgString8", 70, "HALF TIME");
StringConfigItem MsgString9("MsgString9", 70, "BASS TIME");
StringConfigItem MsgString10("MsgString10", 70, "SAUSAGE TIME");
// Define a map of config item names to bits of HTML.
//
// The key should be the same as a config item "name" above.
// The 'id' property of an HTML element should also be the same as the "name" of a config item above (and the same as the key)
// Set the 'name' property of an HTML element to the same as the 'id' in most cases.
//
// Note that {v} will be replaced with the config item value when it is sent to the browser
// The %selected==some value% is used to add 'selected' to the correct choice when rendering a config item as a menu
// Similarly %checked==some value% is used to check/uncheck an item depending on the value of the config item
//
// The order of this array is the order the items will appear on the web page
const char* endMarker = "__END__";

const char* name2html[][2] = {
  {"firsthour", R"(<div>
    <label for='firsthour'>First hour</label>
    <input type='number' min='0' max='23' step='1' id='firsthour' name='firsthour' length=2 value='{v}'>
  </div>)"},

  {"lasthour",  R"(<div>
    <label for='lasthour'>Last hour</label><input type='number' min='0' max='23' step='1' id='lasthour' name='lasthour' length=2 value='{v}'>
  </div>)"},

  {"leadingzero",  R"(
    <div class='checkbox_div'>
    <label for='leadingzero'>Leading zero</label><input type='checkbox' id='leadingzero' name='leadingzero' value='true' %checked==true%>
  </div>)"},

  {"timesystem",  R"(<div>
    <label for='timesystem'>12/24 hour</label>
    <select name='timesystem' id='timesystem'>
      <option value='0' %selected==0%>24 Hour</option>
      <option value='12' %selected==12%>12 Hour</option>
    </select>
  </div>)"},

  {"timezone",  R"(<div>
    <label for='timezone'>Timezone</label>
    <select name='timezone' id='timezone'>
      <option value="Aus Eastern" %selected==Aus Eastern%>Aus Eastern</option>
      <option value="EU Central" %selected==EU Central%>EU Central</option>
      <option value="UK" %selected==UK%>UK</option>
      <option value="US Arizona" %selected==US Arizona%>US Arizona</option>
      <option value="US Central" %selected==US Central%>US Central</option>
      <option value="US Eastern" %selected==US Eastern%>US Eastern</option>
      <option value="US Mountain" %selected==US Mountain%>US Mountain</option>
      <option value="US Pacific" %selected==US Pacific%>US Pacific</option>
      <option value="UTC" %selected==UTC%>UTC</option>
    </select>
  </div>)"},

#ifdef DATE_FORMAT
  {"dateformat",  R"(<div>
    <label for='dateformat'>Date format</label>
    <select name='dateformat' id='dateformat'>
      <option value='0' %selected==0%>ddmmyy</option>
      <option value='1' %selected==1%>mmddyy</option>
      <option value='2' %selected==2%>yymmdd</option>
    </select>
  </div>)"},
#endif

  {"metricUnits",  R"(
    <div class='checkbox_div'>
    <label for='metricUnits'>Metric Units</label><input type='checkbox' id='metricUnits' name='metricUnits' value='true' %checked==true%>
  </div>)"},

    {"PressOffset",  R"(<div>
    <label for='PressOffset'>Pressure Offset in Millbars (for metric) or in inHg/1000 for imperial</label><input type='number' min='0' max='5000' step='1' id='PressOffset' name='PressOffset' length=5 value='{v}'>
  </div>)"},

      {"TempOffset",  R"(<div>
    <label for='TempOffset'>Temperature Offset in Centigrade for Metric or in Fahrenheit for imperial</label><input type='number' min='0' max='10' step='1' id='TempOffset' name='TempOffset' length=4 value='{v}'>
  </div>)"},

  {"NumberFont", R"(<div>
    <label for='firsthour'>Number Font (0-9)</label>
    <input type='number' min='0' max='10' step='1' id='NumberFont' name='NumberFont' length=2 value='{v}'>
  </div>)"},




    {"clockTuner",  R"(<div>
    <label for='clockTuner'>clockTuner (seconds) </label><input type='number' min='0' max='30' step='1' id='clockTuner' name='clockTuner' length=2 value='{v}'>
  </div>)"},

 {"MsgString1",  R"(<div>
    <label for='MsgString1'>Message 1</label><input type='text' id='MsgString1' name='MsgString1' size='70' value='{v}'>
   

  </div>)"}, 
  {"MsgString2",  R"(<div>
    <label for='MsgString2'>Message 2</label><input type='text' id='MsgString2' name='MsgString2' size='70' value='{v}'>
    
     
  </div>)"}, 
  {"MsgString3",  R"(<div>
    <label for='MsgString3'>Message 3</label><input type='text' id='MsgString3' name='MsgString3' size='70' value='{v}'>
    
     
  </div>)"},
   {"MsgString4",  R"(<div>
    <label for='MsgString4'>Message 4</label><input type='text' id='MsgString4' name='MsgString4' size='70' value='{v}'>
    
     
  </div>)"},
   {"MsgString5",  R"(<div>
    <label for='MsgString5'>Message 5</label><input type='text' id='MsgString5' name='MsgString5' size='70' value='{v}'>
   
      
  </div>)"},
   {"MsgString6",  R"(<div>
    <label for='MsgString6'>Message 6</label><input type='text' id='MsgString6' name='MsgString6' size='70' value='{v}'>
    
     
  </div>)"}, 
  {"MsgString7",  R"(<div>
    <label for='MsgString7'>Message 7</label><input type='text' id='MsgString7' name='MsgString7' size='70' value='{v}'>
    
     
  </div>)"},
   {"MsgString8",  R"(<div>
    <label for='MsgString8'>Message 8</label><input type='text' id='MsgString8' name='MsgString8' size='70' value='{v}'>
    
     
  </div>)"},
   {"MsgString9",  R"(<div>
    <label for='MsgString9'>Message 9</label><input type='text' id='MsgString9' name='MsgString9' size='70' value='{v}'>
    
 </div>)"},
  {"MsgString10",  R"(<div>
    <label for='MsgString10'>Message 10</label><input type='text' id='MsgString10' name='MsgString10' size='70' value='{v}'>
    
  </div>)"},

// Always the last one:
  {endMarker, endMarker}
};

#endif
