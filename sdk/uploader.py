import serial,time,sys
'''Sorry for the horrible python code :( '''
PORT="COM4"
FILE= "app.bin"
arg=sys.argv
if ("--help" in arg):
    print("use --port for pointing to port eg:--port COM4 or --port /dev/ttyUSB0")
    print("Use --file for pointing to compiled executable like --file app.bin")
    sys.exit(1)
if "--port" in arg:
    PORT=arg[arg.index("--port")+1]
else:
    print("Port not given using COM4")

if "--file" in arg:
    PORT=arg[arg.index("--file")+1]
else:
    print("File not given using app.bin")

print("Opening port!")
time.sleep(1)
ser=serial.Serial(PORT,115200,timeout=1)
time.sleep(4)
print("Programming!!!")
with open(FILE,"rb") as f:
    b =f.read()
ser.write(b)
if (len(b)<512):
    ser.write(bytes([0]*(512-len(b))))
print("resetting!! please remove the program jumper!")
time.sleep(5)
print("reset")
ser.dtr=False
time.sleep(0.1)
ser.dtr=True
time.sleep(0.5)
print("Program uploaded!")
