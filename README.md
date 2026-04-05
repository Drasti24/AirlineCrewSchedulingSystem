# ✈️ Airline Crew Scheduling System

## 📌 Project Overview
The Airline Crew Scheduling System is a distributed client-server application developed in C++. It allows airline operations staff to manage pilot flight schedules through structured communication between a client application and a central server.

The system supports key scheduling operations such as retrieving pilot schedules, assigning flights, removing flights, updating flights, and downloading large monthly reports.

---

## 🧩 Features

- ✅ Client-server communication using TCP sockets (Winsock)
- ✅ Pilot schedule retrieval
- ✅ Assign flight to pilot
- ✅ Remove flight from pilot
- ✅ Update flight details with validation
- ✅ Download monthly schedule report (>1MB)
- ✅ Real-time download progress display
- ✅ State machine for server-side processing
- ✅ Logging system for TX/RX communication
- ✅ Error handling and input validation

---

## 🏗️ System Architecture

- **Client Application**
  - Handles user input and displays results
  - Sends requests to server
  - Receives and processes responses

- **Server Application**
  - Processes client requests
  - Manages pilot schedules
  - Generates and sends report files
  - Maintains system state using a state machine

---

## 📊 Logging

The system logs:
- All TX/RX packet communication
- Operation responses
- Report download completion
- Error scenarios

Logs are stored in:
- `client_log.txt`
- `server_log.txt`

---

## ⚙️ Technologies Used

- C++ (Visual Studio)
- Winsock2 (TCP communication)
- MSTest (Unit Testing)
- PVS-Studio (Static Analysis)
- File I/O for large data transfer

---

## 🚀 How to Run

1. Open solution in Visual Studio
2. Build the solution
3. Start the server project
4. Run the client project
5. Use the menu to perform scheduling operations

---

## 📌 Notes

- Report files are intentionally large (>1MB) to simulate real-world data transfer.
- The system is designed with reliability and validation in mind, aligning with Software Safety principles.

---

## 👩‍💻 Team Members

- **Drasti Patel** 
- **Komalpreet Kaur**
- **Jiya Pandit**
