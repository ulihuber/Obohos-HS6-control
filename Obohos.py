
while (1):
    snr = input("Serial Number: ")

    if ( (not len(snr) in (7,8))
         or (snr[len(snr)-4]).upper() != "C"):
        print ("Unknown Format")
    else:
        if (len(snr) == 7):
            snr = "0" + snr
        address = "0x" + snr[2:4] + snr[:2] + "0006"
        print("Address:   {}".format(address))

        channel = int(((430.3 + int(snr[6:8])/10)-422.4)*10)
        print("Channel:   0x{0:02x}".format(channel))

        frequency = 422.4 + channel/10
        print("Frequency: {} Mhz".format(frequency)) 
              

    print("--------")
