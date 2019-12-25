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

#include "web_thing.h"

static const char* TAG="web_thing";

/*
Property Type Structure :
{<const char* title>,<const char* property_keyname>,<ThingPropertyValueType value_type>,<bool isRange>,<const char* schema_description>}
*/
#define PropertyTypeInfo_title 			0
#define PropertyTypeInfo_keyname 		1
#define PropertyTypeInfo_ValueType 		2
#define PropertyTypeInfo_IsRange 		3
#define PropertyTypeInfo_TypeSchema		4
const char* PropertyTypeInfo[][5]=
{
	{"Alarm","alarm",BOOLEAN,false,"AlarmProperty"},
	{"Boolean","bool",BOOLEAN,false,"BooleanProperty"},
	{"Brightness","brightness",NUMBER,false,"BrightnessProperty"},
	{"Color","color",STRING,false,"ColorProperty"},
	{"Color Temperature","colortemp",NUMBER,false,"ColorTemperatureProperty"},
	{"Current","current",NUMBER,false,"CurrentProperty"},
	{"Frequency","freq",NUMBER,false,"FrequencyProperty"},
	{"Heating & Cooling","heating",STRING,false,"HeatingCoolingProperty"},
	{"Image","image",NO_STATE,false,"ImageProperty"},
	{"Instantaneous Power","instpower",NUMBER,false,"InstantaneousPowerProperty"},
	{"Leak","leak",BOOLEAN,false,"LeakProperty"},
	{"Level","level",NUMBER,true,"LevelProperty"},
	{"Locked","locked",STRING,false,"LockedProperty"},
	{"Motion","motion",BOOLEAN,false,"MotionProperty"},
	{"On/Off","on",BOOLEAN,false,"OnOffProperty"},
	{"Open/Closed","open",BOOLEAN,false,"OpenProperty"},
	{"Pushed","pushed",BOOLEAN,false,"PushedProperty"},
	{"Target Temperature","targettemp",NUMBER,false,"TargetTemperatureProperty"},
	{"Temperature","temp",NUMBER,false,"TemperatureProperty"},
	{"ThermostatMode","thermostatMode",STRING,false,"ThermostatModeProperty"},
	{"Video","video",NO_STATE,false,"VideoProperty"},
	{"Voltage","voltage",NUMBER,false,"VoltageProperty"},
	{"NULL","NULL",NO_STATE,true,"LAST_PROPERTY"}
};

const char* UnitsToStr[] =
{
	"none",
	"celcius",
	"FARHENITE",
	"PERCENT",
	"MILLISECONDS"
};

Thing* createThing(const char* _title, char** _type)
{
	Thing* thing = malloc(sizeof(Thing));

	// we will wifi mac id as ID 
	thing->id = malloc(sizeof(char)*13);// MAC ID is 12+1 characters long    
	uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    sprintf(thing->id, "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	thing->title = malloc(sizeof(char)*(strlen(_title)+1));
	thing->type = _type;
	strcpy(thing->title,_title);
	thing->property = NULL;
	return thing;
}

ThingProperty* createProperty(ThingPropertyType _type,ThingPropertyValue _defaultValue,bool _isReadOnly,PropertyUnits _units,double _min,double _max,PropertyChange_cb _callback)
{
	ThingProperty* property = malloc(sizeof(ThingProperty));
	property->type = _type;
	property->minimum = _min;
	property->maximum = _max;
	property->readOnly = _isReadOnly;
	property->unit = _units;
	property->next = NULL;
	property->propertyEnum = NULL;
	property->multipleOf = -1;
	property->callback = _callback;

	property->valueType = (ThingPropertyValueType)PropertyTypeInfo[property->type][PropertyTypeInfo_ValueType];
	if(property->valueType == STRING)
	{
		property->value.string = malloc(sizeof(char)*(strlen(_defaultValue.string)+1));
		strcpy(property->value.string,_defaultValue.string);
	}
	else
	{
		property->value = _defaultValue;
	}
	return property;	
}

void addProperty(Thing* _thing,ThingProperty* _property)
{
	//ESP_LOGI(TAG,"In addingproperty:%d",(int)_property->type);
	ThingProperty** nextproperty = &(_thing->property);
	
	while(*nextproperty != NULL)
	{
		nextproperty = &(((ThingProperty*)*nextproperty)->next);
		ESP_LOGI(TAG,"In addProperty fetching next property");
	}
	*nextproperty = _property;	
	if(_property->next == NULL)
		ESP_LOGI(TAG,"next property os NULL");
}

void cleanUpProperty(ThingProperty** property)
{
	if(((ThingProperty*)*property)->valueType == STRING)
	{
		if(((ThingProperty*)*property)->value.string)
		{
			free(((ThingProperty*)*property)->value.string);
		}
	}

	while(((ThingProperty*)*property)->next != NULL)
	{
		cleanUpProperty(&(((ThingProperty*)*property)->next));
	}

	free(((ThingProperty*)*property));	
	*property = NULL;
}

void cleanUpThing(Thing* thing)
{
	//ESP_LOGI(TAG,"In cleanUpThing");	
	if(thing->title != NULL)
		free(thing->title);

	if(thing->id != NULL)
		free(thing->id);

	if(thing->property != NULL)
		cleanUpProperty(&(thing->property));
	else
		ESP_LOGI(TAG,"No Property");
}

cJSON* serializePropertyOrEvent(Thing* device,ThingProperty* property)
{
	//ESP_LOGI(TAG,"In serializePropertyOrEvent");
	cJSON* prop = cJSON_CreateObject();
	switch ((int)PropertyTypeInfo[property->type][PropertyTypeInfo_ValueType]) 
	{
		case NO_STATE:
		break;

		case BOOLEAN:
		cJSON_AddStringToObject(prop,"type","boolean");
		break;

		case NUMBER:
		cJSON_AddStringToObject(prop,"type","number");
		break;

		case STRING:
		cJSON_AddStringToObject(prop,"type","string");
		break;

		default:
			ESP_LOGE(TAG,"Unknnown value");
			return NULL;
	}

	if (property->readOnly) 
	{
		cJSON_AddTrueToObject(prop,"readOnly");
	}
	else
	{
		cJSON_AddFalseToObject(prop,"readOnly");
	}

	if (property->unit != eNONE) 
	{
		cJSON_AddStringToObject(prop,"unit",UnitsToStr[property->unit]);
	}

	cJSON_AddStringToObject(prop,"title",get_property_title(property));

	if(get_property_isRange(property))
	{
		if (property->minimum < property->maximum) 
		{
			cJSON_AddNumberToObject(prop,"minimum",property->minimum );
			cJSON_AddNumberToObject(prop,"maximum",property->maximum );
		}
	}

	if (property->multipleOf > 0) 
	{
		cJSON_AddNumberToObject(prop,"multipleOf",property->multipleOf );
	}

	const char **enumVal = property->propertyEnum;
	bool hasEnum = (property->propertyEnum != NULL) && ((*property->propertyEnum) != NULL);

	if (hasEnum) {
		enumVal = property->propertyEnum;
		cJSON* enumJsonArray = cJSON_CreateArray();
		while (property->propertyEnum != NULL && (*enumVal) != NULL)
		{
			cJSON* jsonStr = cJSON_CreateString(*enumVal);
			cJSON_AddItemToArray(enumJsonArray,jsonStr);
			enumVal++;
		}
		cJSON_AddItemToObject(prop,"enum",enumJsonArray);
	}

	cJSON_AddStringToObject(prop,"@type",get_property_typeschema(property));

	int urlLen = strlen("/things/")+strlen(device->id)+strlen("/properties/")+strlen(get_property_keyname(property))+1;
	char* linkUrl = malloc(sizeof(char)*urlLen);
	if(linkUrl == NULL)
	{
		ESP_LOGE(TAG,"ERROR:NO MEMORY");
	}

	sprintf(linkUrl,"/things/%s/properties/%s",device->id,get_property_keyname(property));

	cJSON* linksJsonArray = cJSON_CreateArray();
	cJSON* href = cJSON_CreateObject();
	cJSON_AddStringToObject(href,"href",linkUrl);
	cJSON_AddItemToArray(linksJsonArray,href);
	cJSON_AddItemToObject(prop,"links",linksJsonArray);

	if(linkUrl)
	{
		free(linkUrl);
	}

	return prop;
} 

cJSON* serializeDevice(Thing* thing) 
{
	cJSON* deviceJson = cJSON_CreateObject();
    cJSON_AddStringToObject(deviceJson,"id",thing->id);
    cJSON_AddStringToObject(deviceJson,"title",thing->title);
    cJSON_AddStringToObject(deviceJson,"@context","https://iot.mozilla.org/schemas");
	// TODO: descr["base"] = ???

    // cJSON* nosecSc = cJSON_CreateObject();
    // cJSON_AddStringToObject(nosecSc,"scheme","nosec");
    // cJSON* securityDefinitions = cJSON_CreateObject();
    // cJSON_AddItemToObject(securityDefinitions,"securityDefinitions",nosecSc);

   	cJSON* typeJson = cJSON_CreateArray();
	const char** type = thing->type;
	while ((type != NULL)&&(*type) != NULL) 
	{
		cJSON* jsonStr = cJSON_CreateString(*type);
		cJSON_AddItemToArray(typeJson,jsonStr);
		type++;
	}
	cJSON_AddItemToObject(deviceJson,"@type",typeJson);

   	cJSON* propertiesJson = cJSON_CreateObject();

	ThingProperty* property = thing->property;
	while (property != NULL) 
	{
		cJSON* propertyJson = serializePropertyOrEvent(thing,property);
		cJSON_AddItemToObject(propertiesJson,get_property_keyname(property),propertyJson);
		property = (ThingProperty*)property->next;
	}
	cJSON_AddItemToObject(deviceJson,"properties",propertiesJson);
	return deviceJson;
}


const char* get_property_title(ThingProperty* property)
{
	return PropertyTypeInfo[property->type][PropertyTypeInfo_title];
}

const char* get_property_keyname(ThingProperty* property)
{
	return PropertyTypeInfo[property->type][PropertyTypeInfo_keyname];
}

const char* get_property_typeschema(ThingProperty* property)
{
	return PropertyTypeInfo[property->type][PropertyTypeInfo_TypeSchema];
}

const bool get_property_isRange(ThingProperty* property)
{
	return PropertyTypeInfo[property->type][PropertyTypeInfo_IsRange];
}

cJSON* serialise_property_item(ThingProperty* property)
{
	cJSON* jsonProp = cJSON_CreateObject();
	const char* propertyKeyName = get_property_keyname(property);
	switch(property->valueType)
	{
		case NO_STATE:
		break;

		case BOOLEAN:
			if(property->value.boolean)
				cJSON_AddTrueToObject(jsonProp,propertyKeyName);
			else
				cJSON_AddFalseToObject(jsonProp,propertyKeyName);
		break;

		case NUMBER:
			cJSON_AddNumberToObject(jsonProp,propertyKeyName,property->value.number);
		break;

		case STRING:
			cJSON_AddStringToObject(jsonProp,propertyKeyName,property->value.string);
		break;

		default:
			ESP_LOGE(TAG,"Unknnown value");
			return NULL;
	}
	return jsonProp;
}

bool update_thing_property(ThingProperty* property,cJSON* newvalue)
{
	cJSON* valueItem = cJSON_GetObjectItem(newvalue,get_property_keyname(property));
	char* newstr = NULL;
	switch(property->valueType)
	{
		case BOOLEAN:
			property->value.boolean = cJSON_IsTrue(valueItem);
		break;

		case NUMBER:
			property->value.number = valueItem->valueint;
		break;

		case STRING:
			newstr = cJSON_GetStringValue(valueItem);
			if(newstr)
			{
				property->value.string = (char*)realloc(property->value.string,strlen(newstr));
				if(property->value.string != NULL)
				{
					strcpy(property->value.string,newstr);
				}
				else
				{
					return false;
				}
			}
		break;

		default:
			ESP_LOGI(TAG,"unknown value");
			return false;
	}
	
	if(property->callback)
	{
		property->callback(property->value);
	}
	return true;
}
