import serial
import serial.tools.list_ports
from tkinter import *
import tkinter.messagebox
from tkinter import simpledialog, filedialog, Tk, Label, Button, Radiobutton, IntVar, ttk
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import (FigureCanvasTkAgg, NavigationToolbar2Tk)
from datetime import datetime
import time
import xlsxwriter
import numpy as np
def readlines():
    global ser
    lines=ser.readlines()
##    print(lines)
    return([[float(j) for j in i.decode("UTF-8")[:-1].split(" ")] for i in lines if len(i[:-1].split(b" "))==3 and b'' not in i[:-1].split(b" ")])
def send(message):
    global ser
    ser.write(message)
def smooth(oy,s):
    v=(s-1)//2
    return oy[:v]+[sum(oy[i-v:i+v+1])/s for i in range(v,len(oy)-v)]+oy[-v:]
def clear():
    global fig,canvas
    if fig!=None:
        fig.clear()
        canvas.draw()
    send(b"1 0 1")
def plot():
    global testInfo,fig,canvas,data
    if "parameters" not in testInfo:
        tkinter.messagebox.showinfo(title="Error", message="Test has not yet been run")
        return
    clear()
    r = sorted(readlines(),key=lambda c:c[-1])
    [f,a,x] = [[j[i] for j in r] for i in range(3)]
    if testInfo["entered"]:
        fig.suptitle('{} - {} - {}'.format(testInfo['name'],testInfo['region'],testInfo['time']))
    d=[x,smooth(f,5),smooth(a,5)]
    startTime=d[0][0]
    for i in range(len(d[0])):
        d[0][i]-=startTime
        d[0][i]/=1000
        d[2][i]=abs(d[2][i])
        a[i]=abs(a[i])
    end=-1
    if testInfo["parameters"]["type"]=="length":
        while abs(x[end]-x[0])>=testInfo["parameters"]["duration"]:
            end-=1
    else:
        while abs(x[end]-x[0])>=testInfo["parameters"]["duration"]/.35:
            end-=1
    if np.polyfit(d[0][:end],d[2][:end],1)[0]>0:
        d[2]=[-i for i in d[2]]
        a=[-i for i in a]
    data=list(zip(d[0],d[1],d[2],f,a))[:end]
##    print(data)
    plot1 = fig.add_subplot(211)
    plot1.plot(d[0][:end],f[:end],alpha=.3)
    plot1.plot(d[0][:end],d[1][:end])
    plot1.set_title("Force")
    plot2 = fig.add_subplot(212)
    plot2.plot(d[0][:end],a[:end],alpha=.3)
    plot2.plot(d[0][:end],d[2][:end])
    plot2.set_title("Angle")
    canvas.draw()
def ask_multiple_choice_question(prompt, options):
    mroot = Tk()
    if prompt:
        Label(mroot, text=prompt).grid(row=0)
    v = IntVar()
    for i, option in enumerate(options):
        r=Radiobutton(mroot, text=option, variable=v, value=i, justify=LEFT, anchor="w")
        r.grid(sticky = W, row=i+1)
    b=Button(master=mroot, text="Submit", command=mroot.destroy)
    b.grid(row=len(options)+1)
    mroot.mainloop()
    if v.get() == 0: return None
    return options[v.get()]
def start():
    global duration,direction,testInfo
    testInfo["parameters"]={"duration":float(duration.get()),"type":"length" if duration.config('relief')[-1] == 'raised' else "time"}
    send(bytes("-1" if direction.config('relief')[-1] == 'sunken' else "1",'utf-8')+b"\n")
    send(bytes(duration.get(),'utf-8')+b"\n")
    send(bytes("1" if durType.config('relief')[-1] == 'sunken' else "2",'utf-8')+b"\n")
    tkinter.messagebox.showinfo(title="Transmission Status", message="Information sent")
def spool():
    send(b"s "+bytes("-1" if direction.config('relief')[-1] == 'sunken' else "1",'utf-8')+b"\n")
    stp=tkinter.messagebox.askokcancel(title="Stop", message="Press OK to stop spooling or Cancel to turn off the device completely")
    send(b"s\n" if stp else b"S\n")
def stop():
    send(b"S")
def askValue(t,m):
    x=None
    while x==None:
        x=simpledialog.askstring(t,m)
    return(x)
def callibrateDC():
    global ser
    numb=int(askValue("DC motor callibration","How many weights: "))
    weights=[askValue("DC motor callibration","{}{} weight: ".format(i,("st" if i%10==1 else "nd") if i%10 in [1,2] else ("rd" if i%10==3 else "th"))) for i in range(1,1+numb)]
    message=b"c "+(b"-1" if direction.config('relief')[-1] == 'sunken' else b"1")+b" "+bytes(str(numb),'utf-8')+b" "+bytes(" ".join(weights),'utf-8')
##    print(message)
    send(message)
    for i in weights[1:]:
        msg=""
        while len(msg:=ser.readlines())<1 or msg[-1]!=b'Next\r\n':
            time.sleep(1)
        stp=tkinter.messagebox.askokcancel(title="Next weight", message="Please put on the {} lb weight and press OK to continue.\n Press cancel to turn off the device completely".format(i))
        send(b"n\n" if stp else b"S\n")
def enterInfo():
    global testInfo, fig
    testInfo['name']=askValue("Test Info","What is the patient's name?")
    testInfo['region']=askValue("Test Info","What is the region of interest?")
    for i in range(3):
        testInfo['patient info {}'.format(i+1)]=askValue('Test Info', 'Patient info {}: '.format(i+1))
    testInfo['time']=datetime.now().strftime("%m/%d/%Y %H:%M")
    testInfo['entered']=True
    if fig!=None:
        fig.suptitle('{} - {} - {}'.format(testInfo['name'],testInfo['region'],testInfo['time']))
def export():
    global data, testInfo
    if data==None:
        tkinter.messagebox.showinfo(title="Error", message="There is no data to export")
    if not testInfo['entered']:
        enterInfo()
    directory=filedialog.askdirectory()
    workbook=xlsxwriter.Workbook("{}/{}.xlsx".format(directory,testInfo["name"]))
    worksheet=workbook.add_worksheet("Testing of {0}".format(testInfo["region"],testInfo["time"]))
    for i in range(3):
        worksheet.write(0,i,'Patient info {}'.format(i+1))
        worksheet.write(1,i,testInfo['patient info {}'.format(i+1)])
    worksheet.write(0,3,"Time of test")
    worksheet.write(1,3,testInfo['time'])
    worksheet.write(2,0,"Time")
    worksheet.write(2,1,"Force (Smooth)")
    worksheet.write(2,2,"Angle (Smooth)")
    worksheet.write(2,3,"Force (Raw)")
    worksheet.write(2,4,"Angle (Raw)")
    for i in range(len(data)):
        for j in range(len(data[i])):
            worksheet.write(i+3,j,data[i][j])
    workbook.close()
ser=serial.Serial()
ser.baudrate=115200
connected=False
while not connected:
    ports=[p for p in serial.tools.list_ports.comports()]
    arduinoFound=False
    for p in ports:
        if "Arduino Uno" in str(p):
            arduinoFound=True
            port=str(p)
    if not arduinoFound:
        port=ask_multiple_choice_question("Choose your Arduino's Port:",sorted(ports))
    ser.port=str(port).split()[0]
    ser.timeout=2
    startTime=datetime.now()
    while not connected and (datetime.now()-startTime).total_seconds()<20:
        connected=True
        try:
            ser.open()
        except Exception:
            connected=False
    if not connected:
        tkinter.messagebox.showerror(title="Error", message="Something went wrong, please try again")
##print(port)
if readlines()!=[[0.0,0.0,0.0]]:
    tkinter.messagebox.showerror(title="Error", message="Something went wrong, please try again")
window = Tk()
window.title('Plotting Serial Monitor')
window.geometry("400x525")
fig = Figure(figsize = (10, 10), dpi = 40)
canvas = FigureCanvasTkAgg(fig,master = window)
canvas.get_tk_widget().grid(row=0,rowspan=3,column=0,columnspan=4,sticky="NSEW")
start_button = Button(master = window, 
                     command = start,
                     text = "Start")
start_button.grid(row=3,column=0,sticky="NSEW")
Label(window,text="Duration:").grid(row=4,sticky="NSEW")
duration=Entry(window)
duration.grid(row=4,column=1,sticky="NSEW")
durType=Button(text="Time (s)", relief="raised", command=lambda:(durType.config(relief=(('raised' if durType.config('relief')[-1] == 'sunken' else 'sunken'))),durType.config(text=('Length (in)' if durType.config('relief')[-1] == 'sunken' else 'Time (s)'))))
durType.grid(row=4,column=2,sticky="NSEW")
Label(window,text="Direction:").grid(row=5,sticky="NSEW")
direction=Button(text="forwards", relief="raised", command=lambda:(direction.config(relief=(('raised' if direction.config('relief')[-1] == 'sunken' else 'sunken'))),spool_button.config(text=('Retract in' if direction.config('relief')[-1] == 'sunken' else 'Spool out')),direction.config(text=('backwards' if direction.config('relief')[-1] == 'sunken' else 'forwards'))))
direction.grid(row=5,column=1,sticky="NSEW")
data=None
plot_button = Button(master = window, 
                     command = plot,
                     text = "Plot")
plot_button.grid(row=3,column=1,sticky="NSEW")
clear_button = Button(master = window, 
                     command = clear,
                     text = "Clear")
clear_button.grid(row=3,column=2,sticky="NSEW")
callibrate_DC_button = Button(master = window, 
                     command = callibrateDC,
                     text = "Callibrate")
callibrate_DC_button.grid(row=5,column=2,sticky="NSEW")
testInfo={'entered':False}
enter_info_button = Button(master=window,
                    command=enterInfo,
                    text="Enter Info")
enter_info_button.grid(row=3,column=3,sticky="NSEW")
export_data_button = Button(master=window,
                    command=export,
                    text="Export Data")
export_data_button.grid(row=4,column=3,sticky="NSEW")
spool_button = Button(master = window, 
                     command = spool,
                     text = "Spool Out",)
spool_button.grid(row=5,column=3,sticky="NSEW")
tkinter.Frame(window,bg="black",height=5,bd=0).grid(row=6,column=0,columnspan=4,sticky="EW")
estop_button = Button(master = window, 
                     command = stop,
                     text = "E Stop",
                     bg="red",fg="white")
estop_button.grid(row=7,column=0,columnspan=4,sticky = 'NSWE')
for i in range(8):
    if i!=6:
        window.rowconfigure(i, weight=3)
for i in range(4):
    window.columnconfigure(i, weight=1)
window.mainloop()
