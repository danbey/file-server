#include "serverconnection.h"
#include "ftpserver.h"


// Destructor, clean up all the mess
serverconnection::~serverconnection() {
    std::cout << "Connection terminated to client (connection id " << this->connectionId << ")" << std::endl;
    delete this->fo;
    close(this->fd);
    this->directories.clear();
    this->files.clear();
}

// Constructor
serverconnection::serverconnection(std::map<int, struct file_locks *> &fileIdToLocksRef, int filedescriptor, unsigned int connId, std::string defaultDir, std::string hostId, unsigned short commandOffset) :
 fileIdToLocks(fileIdToLocksRef), fd(filedescriptor), connectionId(connId), dir(defaultDir), hostAddress(hostId), commandOffset(commandOffset), closureRequested(false), uploadCommand(false), putCommand(false), downloadCommand(false),  receivedPart(0), parameter("") {
//    this->files = std::vector<std::string>();
    this->fo = new fileoperator(this->dir); // File and directory browser
    std::cout << "Connection to client '" << this->hostAddress << "' established" << std::endl;
}

// Check for matching (commands/strings) with compare method
bool serverconnection::commandEquals(std::string a, std::string b) {
    // Convert to lower case for case-insensitive checking
    std::transform(a.begin(), a.end(),a.begin(), toupper);
    int found = a.find(b);
    return (found!=std::string::npos);
}

// Command switch for the issued client command, only called when this->command is set to 0
std::string serverconnection::commandParser(std::string command, std::string fileName) {
    this->parameter;
    std::string res = "";
    this->uploadCommand = false;
    this->putCommand = false;
    struct stat Status;
    // Commands can have either 0 or 1 parameters, e.g. 'browse' or 'browse ./'
    //std::vector<std::string> commandAndParameter = this->extractParameters(command);
//    for (int i = 0; i < parameters.size(); i++) std::cout << "P " << i << ":" << parameters.at(i) << ":" << std::endl;
    std::cout << "Connection " << this->connectionId << ": ";

    /// @TODO: Test if prone to DOS-attacks (if loads of garbage is submitted)???
    // If command with no argument was issued
        if (this->commandEquals(command, "GET")) {
            this->downloadCommand = true;
#if 0
            auto mit = fileIdToLocks.find(atoi(fileName.c_str()));
            if (mit == fileIdToLocks.end()) {
                //TODO need to handle this error
                return ""; 
            }
                       
            mit->second->mutex_reader.lock();
            mit->second->count_reader++;
            if (mit->second->count_reader == 1)
                mit->second->mutex_writer.lock();
            mit->second->mutex_reader.unlock();
#endif

            std::cout << "Preparing download of file '" << fileName << "'" << std::endl;
            unsigned long lengthInBytes = 0;
            std::string fileBlock;
            unsigned long readBytes = 0;
            //std::stringstream st;
            if (!this->fo->readFile(fileName)) {
                // Read the binary file block-wise
//                do {
                    //st.clear();
                   // this->fo->readFileBlock(lengthInBytes, fileblock);
                    //st << lengthInBytes;
                    //readBytes += lengthInBytes;
//                    this->sendToClient(st.str()); // First, send length in bytes to client
                    this->sendToClient(this->fo->getCurrentFileInString()); // This sends the binary char-array to the client
//                    fileoperator* fn = new fileoperator(this->dir);
//                    fn->writeFileAtOnce("./test.mp3",fileBlock);
//                } while (lengthInBytes <= readBytes);
            }

#if 0
            mit->second->mutex_reader.lock();
            mit->second->count_reader--;
            if (mit->second->count_reader == 0)
                mit->second->mutex_writer.unlock();
            mit->second->mutex_reader.unlock();
#endif

            this->closureRequested = true; // Close connection after transfer
        }
        else if (this->commandEquals(command, "UPDATE")) {

#if 0
            auto mit = fileIdToLocks.find(atoi(fileName.c_str()));
            if (mit == fileIdToLocks.end()) {
                // TODO: need to handle this error
                return ""; 
            }
            mit->second->mutex_writer.lock();
#endif
            this->uploadCommand = true; // upload hit!
            std::cout << "Preparing upload of file '" << fileName << "'" << std::endl;
            // all bytes (=parameters[2]) after the upload <file> command belong to the file
            res = this->fo->beginWriteFile(fileName);
//          res = (this->fo->beginWriteFile(this->parameter) ? "Upload failed" : "Upload successful");
        }
        else if (this->commandEquals(command, "PUT")) { 
            this->putCommand = true; // upload hit!
            std::cout << "Preparing upload of file '" << fileName << "'" << std::endl;
            // all bytes (=parameters[2]) after the upload <file> command belong to the file
            
            res = this->fo->beginWriteFile(fileName);
//          res = (this->fo->beginWriteFile(this->parameter) ? "Upload failed" : "Upload successful");
        }
        else if (this->commandEquals(command, "DELETE")) {
            std::cout << "Deletion of file '" << fileName << "' requested" << std::endl;
            this->fo->clearListOfDeletedFiles();
            if (this->fo->deleteFile(fileName)) {
                res = "//";
            } else {
                std::vector<std::string> deletedFile = this->fo->getListOfDeletedFiles();
                if (deletedFile.size() > 0)
                    res = deletedFile.at(0);
            }
        } else {
        // Unknown / no command
            std::cout << "Unknown command encountered!" << std::endl;
            //commandAndParameter.clear();
            //command = "";
            res = "ERROR: Unknown command!";
        }
   
    res += "\n";
    return res;
}

#if 0
// Extracts the command and parameter (if existent) from the client call
std::vector<std::string> serverconnection::extractParameters(std::string command) {
    std::vector<std::string> res = std::vector<std::string>();
    std::size_t previouspos = 0;
    std::size_t pos;
    // First get the command by taking the string and walking from beginning to the first blank
    if ((pos = command.find(SEPARATOR, previouspos)) != std::string::npos) { // No empty string
        res.push_back(command.substr(int(previouspos),int(pos-previouspos))); // The command
//        std::cout << "Command: " << res.back();
    }
    if (command.length() > (pos+1)) {
        //For telnet testing commandOffset = 3 because of the enter control sequence at the end of the telnet command (otherwise = 1)
        res.push_back(command.substr(int(pos+1),int(command.length()-(pos+(this->commandOffset))))); // The parameter (if existent)
//        res.push_back(command.substr(int(pos+1),int(command.length()-(pos+3)))); // The parameter (if existent)
//        std::cout << " - Parameter: '" << res.back() << "'" << std::endl;
    }
    return res;
}
#endif

// Receives the incoming data and issues the apropraite commands and responds
void serverconnection::respondToQuery() {
    char buffer[BUFFER_SIZE];
    int bytes;
    json builder_writer;
    bytes = recv(this->fd, buffer, sizeof(buffer), 0);

    std::string recv_str = std::string(buffer, bytes);

    json root = json::parse(buffer);

    const std::string clientCommand = root["command"].get<std::string>();
    const std::string file_name = root["ID"].get<std::string>();
    const std::string file_data = root["data"].get<std::string>();

    int dan = file_data.size();
    // In non-blocking mode, bytes <= 0 does not mean a connection closure!
    if (file_data.size() > 0) {
        if (this->uploadCommand || this->putCommand) { // (Previous) upload command
            /// Previous (upload) command continuation, store incoming data to the file
            std::cout << "Part " << ++(this->receivedPart) << ": ";
            this->fo->writeFileBlock(file_data);
        } else {
            // If not upload command issued, parse the incoming data for command and parameters
            std::string res = this->commandParser(clientCommand, file_name);
            if (this->uploadCommand || this->putCommand) {
                std::cout << "Part " << ++(this->receivedPart) << ": ";
                this->fo->writeFileBlock(file_data);
            }
            root["data"] = "";
            root["status"] = "200";
            const std::string res_to_query = root.dump();
            this->sendToClient(res_to_query); // Send response to client if no binary file
        }
    } else { // no bytes incoming over this connection
        if (this->uploadCommand || this->putCommand) { // If upload command was issued previously and no data is left to receive, close the file and connection
            
            this->fo->closeWriteFile();

#if 0            
            if (this->uploadCommand) {

                //std::map<int, struct file_locks>::iterator mit =
                auto mit = 
                    fileIdToLocks.find(atoi(file_name.c_str()));
                if (mit == fileIdToLocks.end()) {
                    //TODO need to handle this error
                    return; 
                }
                       
                mit->second->mutex_writer.unlock();
            }
            else if (this->putCommand) {
                auto pair = std::make_pair(1, new file_locks());
                fileIdToLocks.insert(pair);
            }
#endif
            this->uploadCommand = false;
            this->downloadCommand = false;
            this->closureRequested = true;
            this->receivedPart = 0;
        }
        //if (this->downloadCommand) {

        //}

    }
    
}

// Sends the given string to the client using the current connection
void serverconnection::sendToClient(char* response, unsigned long length) {
    // Now we're sending the response
    unsigned int bytesSend = 0;
    while (bytesSend < length) {
        int ret = send(this->fd, response+bytesSend, length-bytesSend, 0);
        if (ret <= 0) {
            return;
        }
        bytesSend += ret;
    }
}

// Sends the given string to the client using the current connection
void serverconnection::sendToClient(const std::string &response) {
    // Now we're sending the response
    unsigned int bytesSend = 0;
    //Json::Value root;
    
    while (bytesSend < response.length()) {
        int ret = send(this->fd, response.c_str()+bytesSend, response.length()-bytesSend, 0);
        if (ret <= 0) {
            return;
        }
        bytesSend += ret;
    }
}

// Returns the file descriptor of the current connection
int serverconnection::getFD() {
    return this->fd;
}

// Returns whether the connection was requested to be closed (by client)
bool serverconnection::getCloseRequestStatus() {
    return this->closureRequested;
}

unsigned int serverconnection::getConnectionId() {
    return this->connectionId;
}
