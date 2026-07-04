# examRank06

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
1. Open a new terminal and type: 
	```bash
	nc localhost 8081
	```
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

### How to Check and Debug
#### Leak Checks
1. Memory Leaks
	```bash
	valgrind --leak-check=full ./mini_serv 8081
	```

2. Fd Leaks (2 ways)
	- **The live way to watch the server while it is running**
		```bash
		ps aux | grep mini_serv
		lsof -p <PID>
		# It helps you see if the file descriptor count is climbing in real-time
		# lsof (List Open Files)
		# What to look for: If you see the number of lines growing every time a client connects and disconnects, you have an fd leak.  
		```
	- **The definitive way - much better and more professional**
		```bash
		valgrind --leak-check=full --track-fds=yes ./mini_serv <port>
		# Tracks the lifecycle of every file descriptor from the moment it's opened until it's closed
		# and tells you exactly where you failed to close one.
		```

#### Check with fcntl?

subject says<br>
```
Hint: To test, you can use fcntl(fd, F_SETFL, O_NONBLOCK) but use select or poll and NEVER check EAGAIN (man 2 send)
```

##### Hint Meaning?

* **The "Never Check EAGAIN" part:** When you use `poll`, you are asking the OS to tell you exactly when a socket is ready to be read from or written to. Because `poll` only triggers when data is *actually* ready, you should never run into an `EAGAIN` error (which happens when you try to read/write on a socket that isn't ready).
* **Why `fcntl` is optional:** You *could* set your sockets to non-blocking mode with `fcntl`, but since `poll` already guarantees the socket is ready before you call `recv` or `send`, your code will work perfectly fine even with standard blocking sockets.
* **Conclusion**: 
	1. You do **not** need to add `fcntl` to your code. Your current implementation of `mini_serv.c` is already handling this the "smart" way:
		- **`poll` is your Guard:** By checking `fds[i].revents & POLLIN` inside your `while(1)` loop, you are ensuring that `recv` is only called when there is guaranteed data waiting in the buffer.
		- **No `EAGAIN` Risks:** Because you only call `recv` inside that `if` block, the program will pause at `poll` and wait for the OS to wake it up only when the network has delivered the data.
		- **Why to skip `fcntl`:** If you were to use `O_NONBLOCK` without `poll`, your code would constantly loop and check sockets, wasting CPU. Since you have `poll`, you have the best of both worlds: efficient waiting and safe reading.

	2. Ignore the `fcntl` part. Your `mini_serv.c` structure—using `poll` to trigger `recv` only when `POLLIN` is set—is the standard, correct way to solve this project without needing to worry about `EAGAIN` or non-blocking settings.



---

### Recommendations
To keep things simple and avoid headaches
- **Stick to a "safe" port:** Use a number like **8081, 8888, or 9000**. These are high enough to avoid system restrictions and low enough to be clearly outside the dynamic range.  
- **Clean up:** If you get a "bind failed" error, don't panic. Just change the number by 1 (e.g., from 8081 to 8082) and try again.
- **Check for "Zombie" processes:** If you are worried that your previous test runs are still stuck, you can try closing your terminal window or using `pkill mini_serv` to ensure no hidden instances of your program are still running in the background.
