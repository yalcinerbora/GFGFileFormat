#include "GFGMayaGraphIterator.h"
#include "maya/MDagPath.h"
#include "maya/MFnDagNode.h"
#include <map>
#include <functional>

GFGMayaAlphabeticalDFSIterator::GFGMayaAlphabeticalDFSIterator(const MDagPath& root)
	: remaining()
	, done(false)
{
	remaining.push(root);
	Next();
}

bool GFGMayaAlphabeticalDFSIterator::IsDone() const
{
	return done; //remaining.empty();
}

MDagPath GFGMayaAlphabeticalDFSIterator::CurrentPath() const
{
	return current;
}

void GFGMayaAlphabeticalDFSIterator::Next()
{
	MDagPath item;
	bool isVisited = true;
	do
	{
		// Check Next
		if(!remaining.empty())
		{
			item = remaining.top();
			remaining.pop();
		}
		else
		{
			done = true;
			return;
		}
			
		isVisited = false;
		for(unsigned int i = 0; i < visited.length(); i++)
		{
			if(visited[i] == item.node())
			{
				isVisited = true;
				break;
			}
		}
	} while(isVisited);

	visited.append(item.node());
	current = item;

	// Add Stuff Alphabetically (In reverse since stack pops from top)
	// Ordering should be in a child squence named 'D' 'C' 'A' 'E' 'B'
	// 'E' 'D' 'C' 'B' 'A' Thus iteration sequence will be alphabetical
	auto compFunc = [] (const MString& first, const MString& second)
	{
		return strncmp(first.asChar(),
					   second.asChar(), 
					   (first.length() >= second.length()) ? first.length() : second.length()) > 0;
	};

	MFnDagNode node(current);
	std::map<MString, MDagPath, std::function<bool(const MString&, const MString&)>> sortTable(compFunc);
	for(unsigned int i = 0; i < node.childCount(); i++)
	{
		MDagPath childPath(current);
		childPath.push(node.child(i));

		sortTable.insert(std::make_pair(childPath.partialPathName(), childPath));
	}

	for(auto i = sortTable.begin(); i != sortTable.end(); i++)
	{
		remaining.push(i->second);
		//cout << "Adding to Stack  " << i->first << endl;
	}
}

//----------------------------------//
GFGMayaAlphabeticalBFSIterator::GFGMayaAlphabeticalBFSIterator(const MDagPath& root)
	: remaining()
	, done(false)
{
	remaining.push(root);
	Next();
}

bool GFGMayaAlphabeticalBFSIterator::IsDone() const
{
	return done; //remaining.empty();
}

MDagPath GFGMayaAlphabeticalBFSIterator::CurrentPath() const
{
	return current;
}

void GFGMayaAlphabeticalBFSIterator::Next()
{
	MDagPath item;
	bool isVisited = true;
	do
	{
		// Check Next
		if(!remaining.empty())
		{
			item = remaining.front();
			remaining.pop();
		}
		else
		{
			done = true;
			return;
		}

		isVisited = false;
		for(unsigned int i = 0; i < visited.length(); i++)
		{
			if(visited[i] == item.node())
			{
				isVisited = true;
				break;
			}
		}
	}
	while(isVisited);

	visited.append(item.node());
	current = item;

	// Add Stuff Alphabetically (In reverse since stack pops from top)
	// Ordering should be in a child squence named 'D' 'C' 'A' 'E' 'B'
	// 'E' 'D' 'C' 'B' 'A' Thus iteration sequence will be alphabetical
	auto compFunc = [] (const MString& first, const MString& second) -> bool
	{
		return strncmp(first.asChar(),
					   second.asChar(),
					   (first.length() >= second.length()) ? first.length() : second.length()) < 0;
	};

	MFnDagNode node(current);
	std::map<MString, MDagPath, std::function<bool(const MString&, const MString&)>> sortTable(compFunc);
	for(unsigned int i = 0; i < node.childCount(); i++)
	{
		MDagPath childPath(current);
		childPath.push(node.child(i));

		sortTable.insert(std::make_pair(childPath.partialPathName(), childPath));
	}

	for(auto i = sortTable.begin(); i != sortTable.end(); i++)
	{
		remaining.push(i->second);
		//cout << "Adding to Queue " << i->first << endl;
	}
}