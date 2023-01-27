## 3LEIC04B - Data Link Protocol

> **2022/2023** - 3rd Year, 1st Semester
>
> **Course** - RCOM (Redes de Computadores)
>
> **Project developed by**
> - Anete Pereira (up202008856)
> - Isabel Amaral (up202006677)
> - Milena Gouveia (up202008862)


### Project Description

The goal of this project was to develop a link-layer protocol that would be used by a higher level application to transfer files between two computers connected by a serial port.

### Project Structure

- src/: Source code for the implementation of the link layer and application layer protocols
- include/: Header files of the link layer and application layer protocols
- cable/: Virtual cable program to help test the serial port (this file must not be changed)
- main.c: Main file
- Makefile: Makefile to build the project and run the application
- penguin.gif: Example file to be sent through the serial port

### Instructions to Run the Project

- Compile the application and the virtual cable program using the provided Makefile
- Run the virtual cable program (either by running the executable manually or using the Makefile target):
```
sudo ./bin/cable
sudo make run_cable
```
- Test the protocol without cable disconnections and noise
	- Run the receiver (either by running the executable manually or using the Makefile target):
		```
		./bin/main /dev/ttyS11 rx penguin-received.gif
		make run_tx
		```

	- Run the transmitter (either by running the executable manually or using the Makefile target):
		```
		./bin/main /dev/ttyS10 tx penguin.gif
		make run_rx
		```

	- Check if the file received matches the file sent, using the diff Linux command or using the Makefile target:
		```
		diff -s penguin.gif penguin-received.gif
		make check_files
		```

- Test the protocol with cable disconnections and noise
	- Run receiver and transmitter again
	- Quickly move to the cable program console and press 0 for unplugging the cable, 2 to add noise, and 1 to normal
	- Check if the file received matches the file sent, even with cable disconnections or with noise

### Program Development State

Our application is capable of correctly transfering files of different sizes and types between two computers, either by using the virtual cable program included in the /cable directory or by connecting the two computers using a physical serial cable.

For a more detailed project report (pt), checkout out the pdf [here](./docs/report.pdf).