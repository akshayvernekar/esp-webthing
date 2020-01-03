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
#define PropertyTypeInfo_keyname 		0
#define PropertyTypeInfo_ValueType 		1
#define PropertyTypeInfo_IsRange 		2
#define PropertyTypeInfo_TypeSchema		3
const char* PropertyTypeInfo[][5]=
{
	{"alarm",BOOLEAN,false,"AlarmProperty"},
	{"bool",BOOLEAN,false,"BooleanProperty"},
	{"brightness",NUMBER,true,"BrightnessProperty"},
	{"color",STRING,false,"ColorProperty"},
	{"colortemp",NUMBER,false,"ColorTemperatureProperty"},
	{"current",NUMBER,false,"CurrentProperty"},
	{"freq",NUMBER,false,"FrequencyProperty"},
	{"heating",STRING,false,"HeatingCoolingProperty"},
	{"image",NO_STATE,false,"ImageProperty"},
	{"instpower",NUMBER,false,"InstantaneousPowerProperty"},
	{"leak",BOOLEAN,false,"LeakProperty"},
	{"level",NUMBER,true,"LevelProperty"},
	{"locked",STRING,false,"LockedProperty"},
	{"motion",BOOLEAN,false,"MotionProperty"},
	{"on",BOOLEAN,false,"OnOffProperty"},
	{"open",BOOLEAN,false,"OpenProperty"},
	{"pushed",BOOLEAN,false,"PushedProperty"},
	{"targettemp",NUMBER,false,"TargetTemperatureProperty"},
	{"temp",NUMBER,false,"TemperatureProperty"},
	{"thermostatMode",STRING,false,"ThermostatModeProperty"},
	{"video",NO_STATE,false,"VideoProperty"},
	{"voltage",NUMBER,false,"VoltageProperty"},
	{"NULL",NO_STATE,true,"LAST_PROPERTY"}
};

//TODO: Its not clear from the spec as to how to change the units, revisit again
const char* UnitsToStr[] =
{
	"none",
	"celcius",
	"FARHENHEIT",
	"PERCENT",
	"MILLISECONDS"
};

Thing* createThing(const char* _title, char** _type)
{
	Thing* thing = malloc(sizeof(Thing));

	// Mac ID will be used as device ID 
	thing->id = malloc(sizeof(char)*13);// MAC ID is 12+1 characters long    
	uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    sprintf(thing->id, "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	thing->title = strdup(_title);	
	thing->type = _type;
	thing->property = NULL;
	return thing;
}

ThingProperty* createProperty(char* _title,PropertyInfo _info,PropertyChange_cb _callback)
{
	ThingProperty* property = malloc(sizeof(ThingProperty));
	if(_title)
	{
		property->title = strdup(_title);
	}

	property->info = _info;
	property->info.valueType = get_property_valueType(property);
	if(property->info.valueType == STRING)
	{
		property->info.value.string = strdup(_info.value.string);
	}
	else
	{
		property->info.value = _info.value;
	}

	property->next = NULL;
	property->callback = _callback;

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
	if(((ThingProperty*)*property)->title)
	{
		free(((ThingProperty*)*property)->title);
		((ThingProperty*)*property)->title = NULL;
	}

	if(((ThingProperty*)*property)->info.valueType == STRING)
	{
		if(((ThingProperty*)*property)->info.value.string)
		{
			free(((ThingProperty*)*property)->info.value.string);
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

// TODO: create unique URLs for properties of same type
char* getPropertyEndpointUrl(Thing* device,ThingProperty* property)
{
	int urlLen = strlen("/things/")+strlen(device->id)+strlen("/properties/")+strlen(get_property_keyname(property))+1; 
	ESP_LOGI(TAG,"urlLen=%d",urlLen);
	char* linkUrl = malloc(sizeof(char)*urlLen);
	if(linkUrl == NULL)
	{
		ESP_LOGE(TAG,"ERROR:NO MEMORY");
		return NULL;
	}

	sprintf(linkUrl,"/things/%s/properties/%s",device->id,get_property_keyname(property));
	return linkUrl;
}

char* getThingDescriptionUrl(Thing* device)
{
	int urlLen = strlen("/things/")+strlen(device->id)+1; 
	ESP_LOGI(TAG,"urlLen=%d",urlLen);
	char* linkUrl = malloc(sizeof(char)*urlLen);
	if(linkUrl == NULL)
	{
		ESP_LOGE(TAG,"ERROR:NO MEMORY");
		return NULL;
	}

	sprintf(linkUrl,"/things/%s",device->id);
	return linkUrl;
}

cJSON* serializePropertyOrEvent(Thing* device,ThingProperty* property)
{
	//ESP_LOGI(TAG,"In serializePropertyOrEvent");
	cJSON* prop = cJSON_CreateObject();
	switch (property->info.valueType) 
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

	if (property->info.readOnly) 
	{
		cJSON_AddTrueToObject(prop,"readOnly");
	}
	else
	{
		cJSON_AddFalseToObject(prop,"readOnly");
	}

	if (property->info.unit != eNONE) 
	{
		cJSON_AddStringToObject(prop,"unit",UnitsToStr[property->info.unit]);
	}

	cJSON_AddStringToObject(prop,"title",property->title);

	if(get_property_isRange(property))
	{
		if (property->info.minimum < property->info.maximum) 
		{
			cJSON_AddNumberToObject(prop,"minimum",property->info.minimum );
			cJSON_AddNumberToObject(prop,"maximum",property->info.maximum );
		}
	}

	if (property->info.multipleOf > 0) 
	{
		cJSON_AddNumberToObject(prop,"multipleOf",property->info.multipleOf );
	}

	const char **enumVal = property->info.propertyEnum;
	bool hasEnum = (property->info.propertyEnum != NULL) && ((*property->info.propertyEnum) != NULL);

	if (hasEnum) {
		enumVal = property->info.propertyEnum;
		cJSON* enumJsonArray = cJSON_CreateArray();
		while (property->info.propertyEnum != NULL && (*enumVal) != NULL)
		{
			cJSON* jsonStr = cJSON_CreateString(*enumVal);
			cJSON_AddItemToArray(enumJsonArray,jsonStr);
			enumVal++;
		}
		cJSON_AddItemToObject(prop,"enum",enumJsonArray);
	}

	cJSON_AddStringToObject(prop,"@type",get_property_typeschema(property));
	char* propUri = getPropertyEndpointUrl(device,property);

	cJSON* linksJsonArray = cJSON_CreateArray();
	cJSON* href = cJSON_CreateObject();
	cJSON_AddStringToObject(href,"href",propUri);
	cJSON_AddItemToArray(linksJsonArray,href);
	cJSON_AddItemToObject(prop,"links",linksJsonArray);

	if(propUri)
	{
		free(propUri);
	}

	return prop;
} 

void serializeDevice(Thing* thing,cJSON* deviceJson) 
{
	if(!deviceJson)
	{
		ESP_LOGE(TAG,"No Json object passes in arg");
		return;
	}
    cJSON_AddStringToObject(deviceJson,"id",thing->id);
    cJSON_AddStringToObject(deviceJson,"title",thing->title);
    cJSON_AddStringToObject(deviceJson,"@context","https://iot.mozilla.org/schemas");
	// TODO: descr["base"] = ???

    cJSON* secScheme = cJSON_CreateObject();
    cJSON_AddStringToObject(secScheme,"scheme","nosec");
    cJSON* noSecSc = cJSON_CreateObject();
    cJSON_AddItemToObject(noSecSc,"nosec_sc",secScheme);
    cJSON_AddItemToObject(deviceJson,"securityDefinitions",noSecSc);

    cJSON_AddStringToObject(deviceJson,"security","nosec_sc");

   	cJSON* linksArrayJson = cJSON_CreateArray();

   	//adding properties link
   	cJSON* propertiesLinkJson = cJSON_CreateObject();
    cJSON_AddStringToObject(propertiesLinkJson,"rel","properties");

    char* propUrl = (char*)malloc(sizeof(char)*(strlen("/things/")+strlen(thing->id)+strlen("/properties")+1));
    if(!propUrl)
    {
    	ESP_LOGE(TAG,"No memory for propUrl");
    	cJSON_Delete(deviceJson);
    	return NULL;
    }
    sprintf(propUrl,"/things/%s/properties",thing->id);
    cJSON_AddStringToObject(propertiesLinkJson,"href",propUrl);

	cJSON_AddItemToArray(linksArrayJson,propertiesLinkJson);
	cJSON_AddItemToObject(deviceJson,"links",linksArrayJson);

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


// const char* get_property_title(ThingProperty* property)
// {
// 	return PropertyTypeInfo[property->info.type][PropertyTypeInfo_title];
// }

const char* get_property_keyname(ThingProperty* property)
{
	return PropertyTypeInfo[property->info.type][PropertyTypeInfo_keyname];
}

const char* get_property_typeschema(ThingProperty* property)
{
	return PropertyTypeInfo[property->info.type][PropertyTypeInfo_TypeSchema];
}

const bool get_property_isRange(ThingProperty* property)
{
	return PropertyTypeInfo[property->info.type][PropertyTypeInfo_IsRange];
}

const ThingPropertyValueType get_property_valueType(ThingProperty* property)
{
	return (ThingPropertyValueType)PropertyTypeInfo[property->info.type][PropertyTypeInfo_ValueType];
}

void serialise_property_item(ThingProperty* property,cJSON* jsonProp)
{
	if(!jsonProp)
	{
		ESP_LOGE(TAG,"No Json object passes in arg");
		return;
	}	

	const char* propertyKeyName = get_property_keyname(property);
	switch(property->info.valueType)
	{
		case NO_STATE:
		break;

		case BOOLEAN:
			if(property->info.value.boolean)
				cJSON_AddTrueToObject(jsonProp,propertyKeyName);
			else
				cJSON_AddFalseToObject(jsonProp,propertyKeyName);
		break;

		case NUMBER:
			cJSON_AddNumberToObject(jsonProp,propertyKeyName,property->info.value.number);
		break;

		case STRING:
			cJSON_AddStringToObject(jsonProp,propertyKeyName,property->info.value.string);
		break;

		default:
			ESP_LOGE(TAG,"Unknnown value");
	}
}

bool update_thing_property(ThingProperty* property,cJSON* newvalue)
{
	cJSON* valueItem = cJSON_GetObjectItem(newvalue,get_property_keyname(property));
	char* newstr = NULL;
	switch(property->info.valueType)
	{
		case BOOLEAN:
			property->info.value.boolean = cJSON_IsTrue(valueItem);
		break;

		case NUMBER:
			property->info.value.number = valueItem->valueint;
		break;

		case STRING:
			newstr = cJSON_GetStringValue(valueItem);
			if(newstr)
			{
				if(property->info.value.string)
				{
					free(property->info.value.string);
					property->info.value.string = NULL;
				}

				property->info.value.string = strdup(newstr);
				if(property->info.value.string == NULL)
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
		property->callback(property->info.value);
	}
	return true;
}
