/*
	This file is part of solidity.

	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/
/**
 * Optimiser component that turns complex expressions into multiple variable
 * declarations.
 */

#include <libjulia/optimiser/ExpressionBreaker.h>

#include <libjulia/optimiser/ASTCopier.h>
#include <libjulia/optimiser/ASTWalker.h>
#include <libjulia/optimiser/NameCollector.h>
#include <libjulia/optimiser/Semantics.h>
#include <libjulia/Exceptions.h>

#include <libsolidity/inlineasm/AsmData.h>

#include <libdevcore/CommonData.h>

#include <boost/range/adaptor/reversed.hpp>

using namespace std;
using namespace dev;
using namespace dev::julia;
using namespace dev::solidity;

void ExpressionBreaker::operator()(FunctionalInstruction& _instruction)
{
	breakExpression(_instruction.arguments);
}

void ExpressionBreaker::operator()(FunctionCall& _funCall)
{
	breakExpression(_funCall.arguments);
}

void ExpressionBreaker::operator()(ForLoop& _loop)
{
	(*this)(_loop.pre);
	// Do not visit the condition because we cannot break expressions there.
	(*this)(_loop.post);
	(*this)(_loop.body);
}

void ExpressionBreaker::operator()(Block& _block)
{
	vector<Statement> saved;
	swap(saved, m_statementsToPrefix);

	vector<Statement> modifiedStatements;
	for (size_t i = 0; i < _block.statements.size(); ++i)
	{
		visit(_block.statements.at(i));
		if (!m_statementsToPrefix.empty())
		{
			if (modifiedStatements.empty())
				std::move(
					_block.statements.begin(),
					_block.statements.begin() + i,
					back_inserter(modifiedStatements)
				);
			modifiedStatements += std::move(m_statementsToPrefix);
			m_statementsToPrefix.clear();
		}
		if (!modifiedStatements.empty())
			modifiedStatements.emplace_back(std::move(_block.statements[i]));
	}
	if (!modifiedStatements.empty())
		_block.statements = std::move(modifiedStatements);

	swap(saved, m_statementsToPrefix);
}

void ExpressionBreaker::breakExpression(vector<Expression>& _arguments)
{
	for (int i = _arguments.size() - 1; i >= 0; --i)
	{
		Expression& argument = _arguments[i];
		if (argument.type() == typeid(FunctionalInstruction) || argument.type() == typeid(FunctionCall))
		{
			visit(argument);
			SourceLocation location = locationOf(argument);
			// TODO could use the name of the function parameter as prefix
			string var = m_nameDispenser.newName("");
			m_statementsToPrefix.emplace_back(VariableDeclaration{
				location,
				{{TypedName{location, var, ""}}},
				make_shared<Expression>(std::move(argument))
			});
			argument = Identifier{location, var};
		}
	}
}
