
# importing the requests library 
import requests 

#get IP
IP = input("Enter the device IP :");
PORT = input("Enter port :");
thing_base = "http://" + IP + ":" + PORT + "/"
device = {}

def test_base():
	print("Getting thing description")
	print("Testing base:`\\`")
	res = requests.get(url = thing_base)

	if res.status_code == 200:
		return res.json
	else:
		return None

def test_thingid():
	print("Getting thing description")
	print("Testing base:`\\`")
	res = requests.get(url = thing_base)

	if res.status_code == 200 :
		return res.json
	else :
		return None


device_json = test_base()
if device_json:
	print("BASE_TEST ok")
else:
	print("BASE_TEST Failed")
	exit()

device["id"] = device_json["id"]
device["title"] = device_json["title"]

