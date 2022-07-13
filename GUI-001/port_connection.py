import serial.tools.list_ports
import time

availableDevicesDict = {}

def connectToPort(port):
    if 'Arduino' not in port: return None
    splitPort = port.split(' ')
    commPort = (splitPort[0])
    print('----Attempting to connect to ' + commPort + '----')

    serialComm = serial.Serial(commPort, baudrate=9600, timeout=1)

    print(f'serialComm.inWaiting() = {serialComm.inWaiting()}')

    serialComm.write(b'M') # Model number
    serialComm.flush()
    for i in range(10):
        if (serialComm.inWaiting() != 0): break
        print("Sleep " + str(i))
        time.sleep(.1)
    if (serialComm.inWaiting() == 0):
        print('-:-:-not able to connect with a port-:-:-')
        raise SystemError("Could not open port")
    
    CatVersion = serialComm.readline()

    CatVersion = CatVersion.decode("utf-8")[:-2]
    print("Connected to: " + CatVersion)

    availableDevicesDict[CatVersion] = serialComm
    return serialComm

# print(f'isOpen() = {availableDevicesDict["CR-B00-B00-000"].isOpen()}')
#
#
# TestPacket = bytearray(b'\x41\xFF\x42\xFF\x43\xFF')
# t = bytearray(b'ri1\xffri2\xffri3\xffri4\xffrm1\xffrm2\xffrm3\xffrm4\xffrr1\xffrr2\xffrr3\xffrr4\xffrp1\xffrp2\xffrp3\xffrt11\xffrt12\xffrt21\xffrt22\xffrt23\xffrt31\xffrt32\xffrt33\xffrt41\xffrt42\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff')
#
# print(TestPacket)
# availableDevicesDict["CR-B00-B00-000"].write(TestPacket)

# ser.isOpen():
