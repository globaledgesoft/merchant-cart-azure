### Introduction
This is Inventory Management application using BG95(Qualcomm MDM9205) wireless "LTE for IoT" board

### Pre-requisites
1. Clone/download zip from Github. Replicate the build environment as follows:
- MDM-Others - Download LLVM compiler tools into this folder
- Quectel_BG95_QuecOpen_SDK_Package_V1.0.2 - Extract BG95 SDK contents into this folder

2. Create Azure Ubuntu instance and enable inbound port 8655.

### Setup in Azure Ubuntu Instance
To install NodeJS :
```
	$ curl -sL https://deb.nodesource.com/setup_10.x | sudo -E bash
	$ sudo apt-get install nodejs
```
To install Postgres :
```
    $ sudo apt-get update
    $ sudo apt-get install postgresql postgresql-contrib
```
Create Postgres user ('music_festival', use password 'p@ssw0rd') :
```
    $ createuser --interactive --pwprompt
```
Create Postgres database (createdb -O user dbname) :
```
    $ createdb -O music_festival music_festival 
```

Install NodeJS process Manager
```
$ npm install pm2 -g
```
### Adding web application to azure instance:
Copy **webapp** folder.
```
 $ scp -i <pem file> -r webapp <azure-instance-url>
```

### Adding azure iothub service application to azure instance:
Copy **read-d2c-messages** folder.
```
 $ scp -i <pem file> -r ./read-d2c-messages/ <azure-instance-url>
```


## Run the applications

Important : make sure Inbound port 8655 is allowed

Important : make sure port 5671/5672 are allowed. This is to connect to the amqp server to access azure iothub built-in endpoint for reading D2C messages

Goto following folder: webapp 

```
    $ npm i
    $ node app.js &
```

Goto folder : read-d2c-messages

Set connectionString (var connectionString = 'your azure service connection string';) and run the application:
```
    $ vim ReadDeviceToCloudMessages.js
    $ npm i
    $ node ReadDeviceToCloudMessages.js &
```

To access webapp frontend from browser type url:

```
    http://youazureinstancelink:8655
```

### Setting up the  Quectel EVB:
##### Getting started

The board comes with a firmware pre-installed. This firmware invokes application entry point on boot. This entry point is quectel_task_entry. Application developers can develop applications for BG95 using SDK. Please request Quectel(support@quectel.com) for SDK. The current version of this application is tested on  Quectel_BG95_QuecOpen_SDK_Package_V1.0.2.

Application development can be done on Ubuntu/Windows 7/10 host machine. Applications can be cross-compiled for BG95 using Qualcomm LLVM compiler. We have used Snapdragon LLVM ARM 4.0.12 release. This compiler can be obtained from Qualcomm Createpoint website (https://createpoint.qti.qualcomm.com)

#### Driver/QPST installation

Please install the latest QPST tool. QPST tool is used to download application binary into MDM9205

### Building and Flashing application program

Building and flashing applications is fairly straight-forward. The azure application source is available in folder (some of the include and utils are used from Quectel BG95 SDK):
```
MDM-IoT-SDK
```

Update Azure IotHub device connection string in the file MDM-IoT-SDK/Azure-Shim/AZURE/iothub_client/samples/iothub_ll_telemetry_sample/iothub_ll_telemetry_sample.c

```
static const char* connectionString = "You azure device connection string";
```


##### Building (verified on ubuntu 16.04):-

1. Open command prompt.
2. You'll find the shell script azure_build.sh in Azure-IoT-Release-BG95/Quectel_BG95_QuecOpen_SDK_Package_V1.0.2
3. As mentioned above under "Setting up the  Quectel EVB -> Getting Started" please request for SDK and unzip into Quectel_BG95_QuecOpen_SDK_Package_V1.0.2 folder. Unzip automatically creates a folder with the same name. 
4. Please ensure azure_build.sh from step 2 is available in Quectel_BG95_QuecOpen_SDK_Package_V1.0.2 folder.
5. Before building the application, we need to set compiler toolchain paths in azure_build.sh. The following paths need to be set. For simplicity this blog assumes that LLVM is installed to MDM-Others/Tools. The directory structure inside Tools/LLVM may be different for different LLVM versions/source :

```
TOOL_PATH_ROOT="../MDM-Others/Tools"
TOOLCHAIN_PATH="$TOOL_PATH_ROOT/LLVM/bin"
TOOLCHAIN_PATH_STANDARdS="$TOOL_PATH_ROOT/LLVM/armv7m-none-eabi/libc/include"
LLVMLIB="$TOOL_PATH_ROOT/LLVM/lib/clang/4.0.12/lib"
LLVMLINK_PATH="$TOOL_PATH_ROOT/LLVM/tools/bin"
PYTHON_PATH="/usr/bin/python"
AZURE_ROOT_PATH="../../MDM-IoT-SDK/Azure-Shim/AZURE"

```
AZURE_ROOT_PATH must specify path to MDM-IoT-SDK/Azure-Shim/AZURE (application source folder). Please use relative paths only(i.e. location of your MDM-IoT-SDK folder with respect to your Quectel SDK directory)

6. Run azure_build.sh

This command will automatically build all the .c files in the application folder. Generates a .bin file called quectel_demo_azure.bin. In addition it also generates a file called oem_app_path.ini. These files are generated under main SDK folder Quectel_BG95_QuecOpen_SDK_Package_V1.0.2/bin

```
./azure_build.sh

```


#### Flashing (Windows 7/10):-
1. Connect the board to host machine
2. Open QPST. Navigate to "Ports" tab.
3. Press "Add New Port" and select the port named "USB/QC Data Modem" port. Refer attached Screenshots
3. From QPST main menu click on, "Start Clients"->"EFS Explorer". EFS explorer opens
4. In EFS Explorer window go to menu, "Phone"->"Preferences...". Now click on "Directories" tab. You can find "Initial File System to Display" in this tab. Choose "Alternate" and click "OK". This step is a one-time operation.
5. Now back in the EFS Explorer main window, click on the "Connect" Icon. The board file-system will be displayed
4. From among the directories listed click on /datatx folder
5. Copy quectel_demo_azure.bin, oem_app_path.ini and demo3.cfg (for APN configuration) into this (/datatx) folder
6. Please update the required network configuration in the config file: demo3.cfg
You can specify apn, username and password to connect to your network in demo3.cfg (in the format apn|username|password). If apn, username and password are not applicable  you need not copy this file

```
apn – Access Point Name of the carrier used 
username – Username for the APN if any
password – Password for the APN if any
```
and copy this file into /datatx.

7. Reset the board
### Make GPIO connections 
Place the sensor mat on the Merch Cart 
Connect the wires to the Quectel EVB board to following gpio pins:
| Sensor No.| GPIO Pin Name  | GPIO Unit no.  |
| :---:   | :-: | :-: |
| Sensor_1 | GPIO_79 | J0202 (verified)|
| Sensor_1 | GPIO_78 | J0202 (verified)|
| Sensor_1 | GPIO_77 | J0202 (verified)|
| Sensor_1 | GPIO_76 | J0202 (verified)|
| Sensor_1 | GPIO2 | J0203   (verified) | 

Note:- Connect sensor mat GND to Quectel EVB board GND (J0202 GND) and VCC to VDD_1V8 (J0202 pin2)

### Running Application:-

Connect the power cable to the Quectel EVB. Make sure the power switch is in OFF position.

Connect the serial to USB cable to “Com Debug” to see Quectel EVB logs.
Power on the Quectel EVB board This is a two-step process, 
1. Flip the switch to ON position.
2. Press the power button next to the Micro USB connector 

You will see Green LED turn ON. This means that the Board is ON 
POWER – Red (Constant) 
STATUS – Green (Constant) 
NET_STATUS – Blue (Blinking) 


Launch Chrome Browser on any host PC and navigate to the following link: http://ip_address_of_azure_instance:8655 

### Troubleshoot : 
Mobile Wireless Data connectivity

##### SIM Registration
Please use the following AT cmds after downloading the binary to test SIM registration. In case data connectivity does not work automatically we may need to fall back to GSM/GPRS mode. Use AT port for AT CMDs.
First let's check if the SIM is registered. Use following AT commands:
```
1. AT+CPIN?

2. AT+CSQ

3. AT+CREG?

4. AT+CGREG?

5. AT+CVERSION
```
CSQ needs to be more than 10
CGREG and CREG should have 0,1 (0,1 means registered to the network)

##### Fall back to GSM/GPRS mode
Please run following AT CMDs:
```
at+qcfg="nwscanmode", 1

at+qcfg="nwscanseq", 1
```

This will default to GSM mode for data connectivity and then hopefully succeed in making data call. Please re-check sim registration using above AT CMDs in case data call doesn't succeed 

