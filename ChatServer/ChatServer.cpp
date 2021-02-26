#include <iostream>
#include <uwebsockets/App.h>
#include <map>
using namespace std;

//User information
struct PerSocketData {
	string name;
	unsigned int user_id;
};

map<unsigned int, string> userNames;

const string BROADCAST_CHANNEL = "broadcast";
const string MESSAGE_TO = "MESSAGE_TO::";
const string SET_NAME = "SET_NAME::";
const string OFFLINE = "OFFLINE::";
const string ONLINE = "ONLINE::";

string online(unsigned int user_id) {
	string name = userNames[user_id];
	//check that there is such a user_id in the map!!!
	return ONLINE + to_string(user_id) + "::" + name;
}

string offline(unsigned int user_id) {
	string name = userNames[user_id];
	//check that there is such a user_id in the map!!!
	return OFFLINE + to_string(user_id) + "::" + name;
}

void updateName(PerSocketData* data) {
	userNames[data->user_id] = data->name;
}

void deleteName(PerSocketData* data) {
	userNames.erase(data->user_id);
}

bool isSetName(string message) {
	return message.find(SET_NAME) == 0;
}
string parseName(string message) {
	return message.substr(SET_NAME.size());
}

//getting user id
string parseUserId(string message) {
	string rest = message.substr(MESSAGE_TO.size());
	int pos = rest.find("::");
	return rest.substr(0, pos);
}

/*getting user message*/
string parseUserText(string message) {
	string rest = message.substr(MESSAGE_TO.size());
	int pos = rest.find("::");
	return rest.substr(pos + 2);
}

bool isMessageTo(string message) {
	return message.find(MESSAGE_TO) == 0;
}

string messageFrom(string user_id, string senderName, string message) {
	return "MESSAGE_FROM::" + user_id + "::[" + senderName + "] " + message;
}

int main() {

	unsigned int last_user_id = 10;

	/*Server configuration*/
	uWS::App(). //create a Server without encryptio
		ws<PerSocketData>("/*", {
		/* Settings */
		.idleTimeout = 1200, //sec
			.open = [&last_user_id](auto* ws) { // "Лямбда функция"
				PerSocketData* userData = (PerSocketData*)ws->getUserData();
				userData->name = "UNNAMED";
				userData->user_id = last_user_id++;

				for (auto entry : userNames) {
					ws->send(online(entry.first), uWS::OpCode::TEXT);
				}

				updateName(userData);
				ws->publish(BROADCAST_CHANNEL, online(userData->user_id));

				cout << "\nNew user connected, id = " << userData->user_id << endl;
				cout << "Total user connected: " << userNames.size() << endl;

				string user_channel = "user#" + to_string(userData->user_id);

				ws->subscribe(user_channel); //personal channel
				ws->subscribe(BROADCAST_CHANNEL); //common channel
			},
			.message = [&last_user_id](auto* ws, string_view message, uWS::OpCode opCode) {
				string strMessage = string(message);
				PerSocketData* userData = (PerSocketData*)ws->getUserData();
				string authorId = to_string(userData->user_id);

				if (isMessageTo(strMessage)) {
					string receiverId = parseUserId(strMessage);
					if (stoi(receiverId) <= last_user_id && stoi(receiverId) > 10) {
						string text = parseUserText(strMessage);
						string outgoingMessage = messageFrom(authorId, userData->name, text);

						ws->publish("user#" + receiverId, outgoingMessage, uWS::OpCode::TEXT); //sending a message to the recipient
						ws->send("Message sent", uWS::OpCode::TEXT);//message to send

						cout << "Author #" << authorId << " wrote message to " << receiverId << endl;
					}
					else ws->send("Error, there is no user with ID = " + receiverId, uWS::OpCode::TEXT);
				}
				if (isSetName(strMessage)) {
					string newName = parseName(strMessage);
					if (newName.length() <= 255) {
						userData->name = newName;
						updateName(userData);
						ws->publish(BROADCAST_CHANNEL, online(userData->user_id));

						cout << "\nUser  #" << authorId << " set their name" << endl;
					}
					else ws->send("Error, name too long", uWS::OpCode::TEXT);
				}
			},
			.close = [](auto* ws, int сode, string_view message) {
				PerSocketData* userData = (PerSocketData*)ws->getUserData();
				ws->publish(BROADCAST_CHANNEL, offline(userData->user_id));
				deleteName(userData);

				cout << "\nUser dtsconnected, id = " << userData->user_id << endl;
				cout << "Total user connected: " << userNames.size() << endl;
			}
			})
		/*Server start*/
				.listen(9001, [](auto* listen_socket) {
				if (listen_socket) {
					cout << "Listening on port " << 9001 << endl;
				}
					}).run();  //Launch
}