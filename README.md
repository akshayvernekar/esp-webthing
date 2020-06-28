# esp-webthing
Esp idf component for creating Mozilla WebThing

## Example Projects
For sample implementations look at [esp-webthing_examples](https://github.com/akshayvernekar/esp-webthing_examples.git) .


## Usage
### Create Thing
```c++
Thing* createThing(const char* _title, char** _type)
````
    Parameters:
        _title = human friendly string which describes the device
        _type  = provides the names of schemas for types of capabilities a device supports.
    For more info refer : https://iot.mozilla.org/wot

### Create Property
```c++
ThingProperty* createProperty(char* _title,PropertyInfo _info,PropertyChange_cb _callback)
```
    Parameters:
        _title = human freindly string which describes the property.
        _info  = PropertyInfo structure which represents the property.
        _callback = is the callback function which will be notified whenever controls are changed . 
    callback function needs to have the format `void(*function_name)(ThingPropertyValue)`

Following properties can be created

| Property Type        | ValueType |
|----------------------|-----------|
| eALARM               | BOOLEAN   |
| eBOOLEAN             | BOOLEAN   |
| eBRIGHTNESS          | NUMBER    |
| eCOLOR               | STRING    |
| eCOLOR_TEMPERATURE   | NUMBER    |
| eCURRENT             | NUMBER    |
| eFREQUENCY           | NUMBER    |
| eHEATING_COOLING     | STRING    |
| eIMAGE               | NONE      |
| eINSTANTANEOUS_POWER | NUMBER    |
| eLEAK                | BOOLEAN   |
| eLEVEL               | NUMBER    |
| eLOCKED              | STRING    |
| eMOTION              | BOOLEAN   |
| eON_OFF              | BOOLEAN   |
| eOPEN                | BOOLEAN   |
| ePUSHED              | BOOLEAN   |
| eTARGET_TEMPERATURE  | NUMBER    |
| eTEMPERATURE         | NUMBER    |
| eTHERMOSTAT          | STRING    |
| eVIDEO               | NONE      |
| eVOLTAGE             | NUMBER    |

### PropertyInfo structure
Property Info structure is passed as a parameter for property creation .
```c++
struct PropertyInfo
{
    ThingPropertyType type;
    ThingPropertyValue value;
    ThingPropertyValueType valueType;
    double minimum;
    double maximum;
    double multipleOf;
    bool readOnly;
    PropertyUnits unit;
}
```
    Parameters:
        type = enum representing the property type.
        value = value of the property.
        valueType = value type can be one of the four BOOLEAN,NUMBER, STRING or NONE. 
                Refer the Property Type table for more info.
        minimum = minimum permissible value for the property . 
                Applicable only for properties which have Range type values such as Brightness etc.
        maximum = maximum permissible value for the property . 
                Applicable only for properties which have Range type values such as Brightness etc .
        readOnly = specifies if the value of property can be changed . 
                Set it to TRUE if you dont want the user to change the value of property for example readings of a sensor.
        unit = SI unit of the property .
### Add Property to Thing object
```c++
void addProperty(Thing* _thing,ThingProperty* _property)
```
    Parameters:
        _thing = pointer to the thing object 
        _property  = pointer to property object 

### Adding Property to Thing
```c++
void addProperty(Thing* _thing,ThingProperty* _property)
```
    Parameters:
        _thing = pointer to the thing object 
        _property  = pointer to property object 

### Initialsing Adapter
Initialses mdns with thing details.
```c++
void initAdapter(Thing* thing)
```
    Paremeters:
        thing = pointer to thing object.
Note: Call this before connecting ESP32 to wifi

### Start Adapter
Starts the webserver and initialises the handles .
```c++
void startAdapter(Thing* thing)
``` 
    Paremeters:
        thing = pointer to thing object.
Note : Call this after ESP gets connected to the wifi network

### Cleanup Thing
Frees allocated memory for thing and its properties.
```c++
void cleanUpThing(Thing* thing)
``` 
    Parameters:
        _thing = pointer to the thing object 

## For sample implemnatation see : https://github.com/akshayvernekar/esp-webthing_examples
