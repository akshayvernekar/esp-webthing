/*
  Copyright (c) 2019 Akshay Vernekar

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#ifndef WEB_THING_H
#define WEB_THING_H

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "cJSON.h"

typedef enum ThingPropertyValueType {
  NO_STATE,
  BOOLEAN,
  NUMBER,
  STRING
}ThingPropertyValueType;

typedef union ThingPropertyValue {
  bool boolean;
  double number;
  char* string;
}ThingPropertyValue;

typedef enum PropertyUnits{
	eNONE,
	eCELCUIS,
	eFARHENITE,
	ePERCENT,
	eMILLISECONDS
}PropertyUnits;

typedef enum ThingPropertyType{
	eALARM,
	eBOOLEAN,
	eBRIGHTNESS,
	eCOLOR,
	eCOLOR_TEMPERATURE,
	eCURRENT,
	eFREQUENCY,
	eHEATING_COOLING,
	eIMAGE,
	eINSTANTANEOUS_POWER,
	eLEAK,
	eLEVEL,
	eLOCKED,
	eMOTION,
	eON_OFF,
	eOPEN,
	ePUSHED,
	eTARGET_TEMPERATURE,
	eTEMPERATURE,
	eTHERMOSTAT,
	eVIDEO,
	eVOLTAGE,
	eKEEP_LAST
}ThingPropertyType;

typedef void(*PropertyChange_cb)(ThingPropertyValue);
typedef struct ThingProperty ThingProperty;

typedef struct PropertyInfo
{
	ThingPropertyType type;
	ThingPropertyValue value;
	ThingPropertyValueType valueType;
	double minimum;
	double maximum;
 	double multipleOf;
	bool readOnly;
	PropertyUnits unit;
	const char** propertyEnum;	
}PropertyInfo;

struct ThingProperty
{
	char* title;
	ThingProperty* next;
	PropertyInfo info;
	PropertyChange_cb callback;
};

typedef struct Thing
{
	char* id;
	char* title;
	char** type;
	ThingProperty* property;
}Thing;

/* 	
	Creates a thing.
	Parameters:
		_title = human friendly string which describes the device
		_type  = provides the names of schemas for types of capabilities a device supports.
	For more info refer : https://iot.mozilla.org/wot 
*/
Thing* createThing(const char* _title, char** _type);


/* 
	Creates a property.

	Parameters:
		_type = enum representing type of property to create , the list is based on the schemas 
				present in https://iot.mozilla.org/schemas .
		_defaultValue  = default value of the property
		_isReadOnly = refers if the property is read only
		_units = enum representing the SI unit
		_min/_max = minimum and maximum value in range (applicable to only certain properties). 
		_callback = is the callback function in the main.c which will be notified whenever controls are changed . 
					Callback function needs to have the format void(*function_name)(ThingPropertyValue)
*/
ThingProperty* createProperty(char* title,PropertyInfo info,PropertyChange_cb _callback);

/* Adds property to the thing.

	Parameters:
		_thing = pointer to the thing object 
		_property  = pointer to property object  
 */
void addProperty(Thing* _thing,ThingProperty* _property);


/* 
	Clean up property.

	Deallocates memory allocated to thing and associated properties.
 
	Parameters:
		_thing = pointer to the thing object 
*/
void cleanUpThing(Thing* _thing);


/* 
	Returns a cJSON object representing the thing object .
	Note: Please cleanup the jSON obejct using cJSON_Delete() function
 
	Parameters:
		_thing = pointer to the thing object 
*/
void serializeDevice(Thing* thing,cJSON* deviceJson) ;


/* 
	Returns a cJSON object representing the thing property .
	Note: Please cleanup the jSON obejct using cJSON_Delete() function

	Parameters:
		_property = pointer to the thing property object. 
*/
void serialise_property_item(ThingProperty* property,cJSON* jsonProp);

/* Helper functions to get keyname , title ,typeschema(@type)*/
const char* get_property_keyname(ThingProperty* property);
const char* get_property_title(ThingProperty* property);
const char* get_property_typeschema(ThingProperty* property);
const bool get_property_isRange(ThingProperty* property);
const ThingPropertyValueType get_property_valueType(ThingProperty* property);

/* 
	Updates the current value of the property 
	Paremeters:
		property = pointer to thing property
		newvalue = cJSON object representing the new value to be updated .
*/
bool update_thing_property(ThingProperty* property,cJSON* newvalue);

char* getPropertyEndpointUrl(Thing* device,ThingProperty* property);
char* getThingDescriptionUrl(Thing* device);
#endif