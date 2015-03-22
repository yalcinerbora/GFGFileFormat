/**

GFGMayaAlphabeticalDFSIterator Class
GFGMayaAlphabeticalBFSIterator Class

These classes iterates the DAG in BF or DF manner
When It needs to resolve selection it does it alphabetically

For License refer to:
https://github.com/yalcinerbora/GFGFileFormat/blob/master/LICENSE
*/

#ifndef __GFG_MAYAGRAPHITERATOR_H__
#define __GFG_MAYAGRAPHITERATOR_H__

#include <queue>
#include <stack>
#include <maya/MDagPath.h>
#include <maya/MObjectArray.h>
#include <set>

class GFGMayaAlphabeticalDFSIterator
{
	private:
		bool						done;
		MDagPath					current;
		MObjectArray				visited;
		std::stack<MDagPath>		remaining;

	protected:
	public:
		// Constructors & Destructor
					GFGMayaAlphabeticalDFSIterator(const MDagPath& root);
					~GFGMayaAlphabeticalDFSIterator() = default;

		bool		IsDone() const;
		MDagPath	CurrentPath() const;
		void		Next();
};

class GFGMayaAlphabeticalBFSIterator
{
	private:
		bool						done;
		MDagPath					current;
		MObjectArray				visited;
		std::queue<MDagPath>		remaining;

	protected:
	public:
		// Constructors & Destructor
					GFGMayaAlphabeticalBFSIterator(const MDagPath& root);
					~GFGMayaAlphabeticalBFSIterator() = default;

		bool		IsDone() const;
		MDagPath	CurrentPath() const;
		void		Next();


};
#endif //__GFG_MAYAGRAPHITERATOR_H__