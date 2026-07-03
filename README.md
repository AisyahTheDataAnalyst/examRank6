# examRank06
42 exam rank 06 based on the given main. This version is very solid because it allocates the memory. And it is not too hard to remember because it takes a lot from the `main.c` given during the exam

---
### Initial Notes
- Went to take the exam today (2nd July 2026)
- Files provided within subject/ has been updated and identical as per latest examRank6
<!-- Rest for the day, sleep deprived & overwork for the whole month. Need to go to thr vet to the next day for Tam&Mona -->
---

### What is this question about actually?
- This is a **simplified chat server**. Imagine it as a central post office.
- **The Goal**: 
	- It listens for people (clients) to connect. 
	- When one person sends a message, the server acts as a relay, broadcasting that message to everyone else connected, while adding a label (e.g., client 0: Hello).
- **The Mechanism**: 
	- Because you are handling multiple clients simultaneously without blocking the server, you must use a "multiplexing" function like poll (or select). 
	- These functions allow your program to pause and "watch" a list of active sockets, only waking up when someone sends a message or a new person tries to connect.

---

### How to Run and Test
Since this is a network program, you need two kind of terminal windows:

#### Start the server: 
1. Compile:
	```bash
	gcc -Wall -Wextra -Werror mini_serv.c -o mini_serv
	```
2. Run: 
	```bash
	./mini_serv 8081 
	#(You can use any valid port number)
	```

#### Connect clients (using nc):
1. Open a new terminal and type: nc localhost 8081
2. Open another 1/+ terminals and type the same command to see messages relay between them.
3. This will showcase the "chat system" between all clients with indications of:
- who enter the chat	
	```
	server: client 1 just arrived
	```
- who sent the messages	
	```
	client 0: where makan guys?
	```
- who left the chat
	```
	server: client 0 just left
	```

---

### Recommendations
To keep things simple and avoid headaches
- **Stick to a "safe" port:** Use a number like **8081, 8888, or 9000**. These are high enough to avoid system restrictions and low enough to be clearly outside the dynamic range.  
- **Clean up:** If you get a "bind failed" error, don't panic. Just change the number by 1 (e.g., from 8081 to 8082) and try again.
- **Check for "Zombie" processes:** If you are worried that your previous test runs are still stuck, you can try closing your terminal window or using `pkill mini_serv` to ensure no hidden instances of your program are still running in the background.
