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
ThingProperty* createProperty(ThingPropertyType _type,ThingPropertyValue _defaultValue,bool _isReadOnly,PropertyUnits _units,double _min,double _max,PropertyChange_cb _callback)
```
    Parameters:
        _type = enum representing type of property to create , the list is based on the schemas 
                present in https://iot.mozilla.org/schemas .
        _defaultValue  = default value of the property
        _isReadOnly = refers if the property is read only
        _units = enum representing the SI unit
        _min/_max = minimum and maximum value in range (applicable to only certain properties).
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
| eIMAGE               | NO_STATE  |
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
| eVIDEO               | NO_STATE  |
| eVOLTAGE             | NUMBER    |
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


