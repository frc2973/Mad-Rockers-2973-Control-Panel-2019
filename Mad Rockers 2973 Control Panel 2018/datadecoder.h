
#pragma once

#include <fstream>
#include <string>
#include <vector>

using namespace std;

string getFile(string path) {
	string output;
	ifstream stream(path);
	if (!stream.is_open())
		return "";
	string line;
	while (getline(stream, line)) {
		output += line + "\n";
	}
	stream.close();
	return output;
}

//Probably should be class-inheritence but whatever
class DataDecodeSingleArgStringKey {
public:
	vector<string> keys;
	vector<string> values;
	string lookup(string key) {
		for (int i = 0; i < keys.size();i++) {
			if (keys[i] == key)
				return values[i];
		}
		return "";
	}
	bool decodeFile(string path) {
		keys.clear(); values.clear();
		ifstream stream(path);
		if (!stream.is_open())
			return false;
		string line;
		while (getline(stream,line)) {
			string temp = "";
			if (line == "")
				continue;
			if (line[0] == '/')
				continue;
			bool key = false;
			for (int i = 0; i < line.length(); i++) {
				if (!key) {
					if (temp == "" && line[i] == ' ') {

					}
					else if (line[i] == ' ')
					{
						key = true;
						keys.push_back(temp);
						temp = "";
					}
					else {
						temp += line[i];
					}
				}
				else {
					if (!(temp==""&&line[i]==' '))
						temp += line[i];
				}
			}
			if (temp != "") {
				values.push_back(temp);
			}
		}
		stream.close();
		return true;
	}
};
class DataDecodeSingleArgIntKey {
public:
	vector<int> keys;
	vector<string> values;
	string lookup(int key) {
		for (int i = 0; i < keys.size(); i++) {
			if (keys[i] == key)
				return values[i];
		}
		return "";
	}
	bool decodeFile(string path) {
		keys.clear(); values.clear();
		ifstream stream(path);
		if (!stream.is_open())
			return false;
		string line;
		while (getline(stream, line)) {
			string temp = "";
			if (line == "")
				continue;
			if (line[0] == '/')
				continue;
			bool key = false;
			for (int i = 0; i < line.length(); i++) {
				if (!key) {
					if (temp == "" && line[i] == ' ') {

					}
					else if (line[i] == ' ')
					{
						key = true;
						keys.push_back(stoi(temp));
						temp = "";
					}
					else {
						temp += line[i];
					}
				}
				else {
					if (!(temp == ""&&line[i] == ' '))
						temp += line[i];
				}
			}
			if (temp != "") {
				values.push_back(temp);
			}
		}
		stream.close();
		return true;
	}
};