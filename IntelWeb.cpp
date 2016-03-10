#include "IntelWeb.h"
#include <iostream>
#include <fstream>
#include <sstream>
//#include <string> already in IntelWeb header

using namespace std;

IntelWeb::IntelWeb() {

}

IntelWeb::~IntelWeb() {
	forwardRelations.close();
	reverseRelations.close();
}

bool IntelWeb::createNew(const std::string& filePrefix, unsigned int maxDataItems) {
	close();

	unsigned int numBuckets = maxDataItems / 0.75;
	const std::string& fileName1 = filePrefix + "forward";
	const std::string& fileName2 = filePrefix + "reverse";

	bool success = forwardRelations.createNew(fileName1, numBuckets);
	if (!success) {
		close();
		return false;
	}
	success = reverseRelations.createNew(fileName2, numBuckets);
	if (!success) {
		close();
		return false;
	}
	return success;
}

bool IntelWeb::openExisting(const std::string& filePrefix) {
	close();
	const std::string& fileName1 = filePrefix + "forward";
	const std::string& fileName2 = filePrefix + "reverse";

	bool success = forwardRelations.openExisting(fileName1);
	if (!success) {
		close();
		return false;
	}
	success = reverseRelations.openExisting(fileName2);
	if (!success) {
		close();
		return false;
	}
	return success;
}

void IntelWeb::close() {
	forwardRelations.close();
	reverseRelations.close();
}

bool IntelWeb::ingest(const std::string& telemetryFile) {

	// Open the file for input
	ifstream inf(telemetryFile);
	// Test for failure to open
	if (!inf)
	{
		cout << "Cannot open telemetry file!" << endl;
		return 1;
	}

	string line;
	while (getline(inf, line))
	{
		istringstream iss(line);

		//no need for extra data structures tbh
		string machine;
		string to;
		string from;

		if (!(iss >> machine >> to >> from))
		{
			cout << "Ignoring badly-formatted input line: " << line << endl;
			continue;
		}
		char dummy;
		if (iss >> dummy) // succeeds if there a non-whitespace char
			cout << "Ignoring extra data in line: " << line << endl;

		// Add data to expenses map
		//input in forward: from to
		//input in reverse: to from
		forwardRelations.insert(to, from, machine);
		reverseRelations.insert(from, to, machine);
	}
}

bool IntelWeb::purge(const std::string& entity) {
	bool purged = false;
	//remove from forward relations
	//remove from reverse relations

	return purged;
}

unsigned int IntelWeb::crawl(const std::vector<std::string>& indicators,
	unsigned int minPrevalenceToBeGood,
	std::vector<std::string>& badEntitiesFound,
	std::vector<InteractionTuple>& badInteractions
	) {
	return 0;
}