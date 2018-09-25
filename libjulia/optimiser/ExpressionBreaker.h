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
#pragma once

#include <libjulia/ASTDataForward.h>

#include <libjulia/optimiser/ASTCopier.h>
#include <libjulia/optimiser/ASTWalker.h>
#include <libjulia/optimiser/NameDispenser.h>
#include <libjulia/Exceptions.h>

#include <libevmasm/SourceLocation.h>

#include <boost/variant.hpp>
#include <boost/optional.hpp>

#include <set>

namespace dev
{
namespace julia
{

class NameCollector;


/**
 * Optimiser component that modifies an AST in place, turning complex
 * expressions into simple expressions and multiple variable declarations.
 *
 * Code of the form
 *
 * sstore(mul(x, 4), mload(y))
 *
 * is transformed into
 *
 * let a1 := mload(y)
 * let a2 := mul(x, 4)
 * sstore(a2, a1)
 *
 * The transformation is only applied to expressions at the statement
 * level, i.e. not to conditions.
 */
class ExpressionBreaker: public ASTModifier
{
public:
	explicit ExpressionBreaker(NameDispenser& _nameDispenser):
		m_nameDispenser(_nameDispenser)
	{ }
	~ExpressionBreaker()
	{
		assertThrow(m_statementsToPrefix.empty(), OptimizerException, "");
	}

	virtual void operator()(FunctionalInstruction&) override;
	virtual void operator()(FunctionCall&) override;
	virtual void operator()(ForLoop&) override;
	virtual void operator()(Block& _block) override;

private:
	void breakExpression(std::vector<Expression>& _arguments);

	/// List of statements that should go in front of the currently visited AST element,
	/// at the statement level.
	std::vector<Statement> m_statementsToPrefix;
	NameDispenser& m_nameDispenser;
};

}
}
