// $Id: cix_protocol.cpp,v 1.7 2014-07-24 20:24:51-07 - - $

#include <fstream>
#include <unordered_map>
#include <string>
using namespace std;

#include "cix_protocol.h"

const unordered_map<int,string> cix_command_map {
   {int (CIX_ERROR), "CIX_ERROR"},
   {int (CIX_EXIT ), "CIX_EXIT" },
   {int (CIX_GET  ), "CIX_GET"  },
   {int (CIX_HELP ), "CIX_HELP" },
   {int (CIX_LS   ), "CIX_LS"   },
   {int (CIX_PUT  ), "CIX_PUT"  },
   {int (CIX_RM   ), "CIX_RM"   },
   {int (CIX_FILE ), "CIX_FILE" },
   {int (CIX_LSOUT), "CIX_LSOUT"},
   {int (CIX_ACK  ), "CIS_ACK"  },
   {int (CIX_NAK  ), "CIS_NAK"  },
};

string trim (const string &str) {
   size_t first = str.find_first_not_of(" \t");
   if (first equals string::npos) return "";
   size_t last = str.find_last_not_of("\n");
   string temp1 = str.substr (first, last - first +1);
   last = temp1.find_last_not_of(" ");
   string temp2 = temp1.substr (first, last - first +1);
   return temp2;
}

string load_file (cix_header& header) {
   ifstream is(header.cix_filename, ifstream::binary);
   if(is) {
      is.seekg (0, is.end);
      int length = is.tellg();
      is.seekg (0, is.beg);
      char buffer[length];
      if (length == -1) cout << "found our problem :D" << endl;
      cout << "Reading " << length << " characters... ";
      is.read (buffer, length);
      buffer[length] = '\0';
      is.close();
      string buf(buffer);
      return buf;
   }else {
      //cout << "load_file failed..." << endl;
      header.cix_command = CIX_NAK;
      header.cix_nbytes = errno;
      return nullptr;
   }
}

void write_file (cix_header& header, char* buffer) {
   ofstream os(header.cix_filename, ofstream::binary);
   string buf(buffer);
   int size = buf.size();
   os.write (buffer, size);
   if (os.fail()) {
      //cout <<"WITHIN WRITE_FILE: write failed " << endl;
      header.cix_command = CIX_NAK;
      header.cix_nbytes = errno;
   }else {
      header.cix_command = CIX_ACK;
   }
   os.close();
}

void send_packet (base_socket& socket,
                  const void* buffer, size_t bufsize) {
   const char* bufptr = static_cast<const char*> (buffer);
   ssize_t ntosend = bufsize;
   do {
      ssize_t nbytes = socket.send (bufptr, ntosend);
      if (nbytes < 0) throw socket_sys_error (to_string (socket));
      bufptr += nbytes;
      ntosend -= nbytes;
   }while (ntosend > 0);
}

void recv_packet (base_socket& socket, void* buffer, size_t bufsize) {
   char* bufptr = static_cast<char*> (buffer);
   ssize_t ntorecv = bufsize;
   do {
      ssize_t nbytes = socket.recv (bufptr, ntorecv);
      if (nbytes < 0) throw socket_sys_error (to_string (socket));
      if (nbytes == 0) throw socket_error (to_string (socket)
                                           + " is closed");
      bufptr += nbytes;
      ntorecv -= nbytes;
   }while (ntorecv > 0);
}

ostream& operator<< (ostream& out, const cix_header& header) {
   const auto& itor = cix_command_map.find (header.cix_command);
   string code = itor == cix_command_map.end() ? "?" : itor->second;
   cout << "{" << header.cix_nbytes << "," << code << "="
        << int (header.cix_command) << ",\"" << header.cix_filename
        << "\"}";
   return out;
}    


string get_cix_server_host (const vector<string>& args, size_t index) {
   if (index < args.size()) return args[index];
   char* host = getenv ("CIX_SERVER_HOST");
   if (host != nullptr) return host;
   return "localhost";
}

in_port_t get_cix_server_port (const vector<string>& args,
                               size_t index) {
   string port = "-1";
   if (index < args.size()) port = args[index];
   else {
      char* envport = getenv ("CIX_SERVER_PORT");
      if (envport != nullptr) port = envport;
   }
   return stoi (port);
}
     
