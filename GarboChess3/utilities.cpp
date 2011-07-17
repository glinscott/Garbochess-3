#include "garbochess.h"
#include "position.h"
#include "movegen.h"

#include <string>
#include <vector>
#include <algorithm>

std::vector<std::string> tokenize(const std::string &in, const std::string &tok)
{
	std::string::const_iterator cp = in.begin();
	std::vector<std::string> res;
	while (cp != in.end())
	{ 
		while (cp != in.end() && std::count(tok.begin(), tok.end(), *cp)) cp++;
		std::string::const_iterator tmp = std::find_first_of(cp, in.end(), tok.begin(), tok.end()); 
		if (cp != in.end()) res.push_back(std::string(cp, tmp)); 
		cp = tmp;
	}
	return res;
}

Move MakeMoveFromUciString(Position &position, const std::string &moveString)
{
	const Square from = MakeSquare(RANK_1 - (moveString[1] - '1'), moveString[0] - 'a');
	const Square to = MakeSquare(RANK_1 - (moveString[3] - '1'), moveString[2] - 'a');

	if (moveString.length() == 5)
	{
		int promotionType;
		switch (tolower(moveString[4]))
		{
		case 'n': promotionType = PromotionTypeKnight; break;
		case 'b': promotionType = PromotionTypeBishop; break;
		case 'r': promotionType = PromotionTypeRook; break;
		case 'q': promotionType = PromotionTypeQueen; break;
		}
		return GeneratePromotionMove(from, to, promotionType);
	}

	Move moves[256];
	int moveCount = GenerateLegalMoves(position, moves);
	for (int i = 0; i < moveCount; i++)
	{
		if (GetFrom(moves[i]) == from && GetTo(moves[i]) == to)
		{
			return moves[i];
		}
	}

	ASSERT(false);
	return 0;
}

std::string GetSquareSAN(const Square square)
{
	std::string result;
	result += GetColumn(square) + 'a';
	result += (RANK_1 - GetRow(square)) + '1';
	return result;
}

std::string GetMoveUci(const Move move)
{
	std::string result;
	result += GetSquareSAN(GetFrom(move));
	result += GetSquareSAN(GetTo(move));

	if (GetMoveType(move) == MoveTypePromotion)
	{
		switch (GetPromotionMoveType(move))
		{
		case KNIGHT: result += "n"; break;
		case BISHOP: result += "b"; break;
		case ROOK: result += "r"; break;
		case QUEEN: result += "q"; break;
		}
	}

	return result;
}

std::string GetMoveSAN(Position &position, const Move move)
{
	const Square from = GetFrom(move);
	const Square to = GetTo(move);

	const Move moveType = GetMoveType(move);

	std::string result;
	if (moveType == MoveTypeCastle)
	{
		if (GetColumn(to) > FILE_E)
		{
			result = "O-O";
		}
		else
		{
			result = "O-O-O";
		}
	}
	else
	{
		// Piece that is moving
		const PieceType fromPieceType = GetPieceType(position.Board[from]);
		switch (fromPieceType)
		{
		case PAWN: break;
		case KNIGHT: result += "N"; break;
		case BISHOP: result += "B"; break;
		case ROOK: result += "R"; break;
		case QUEEN: result += "Q"; break;
		case KING: result += "K"; break;
		}

		Move legalMoves[256];
		int legalMoveCount = GenerateLegalMoves(position, legalMoves);

		// Do we need to disambiguate?
		bool dupe = false, rowDiff = true, columnDiff = true;
		for (int i = 0; i < legalMoveCount; i++)
		{
			if (GetFrom(legalMoves[i]) != from &&
				GetTo(legalMoves[i]) == to &&
				GetPieceType(position.Board[GetFrom(legalMoves[i])]) == fromPieceType)
			{
				dupe = true;
				if (GetRow(GetFrom(legalMoves[i])) == GetRow(from))
				{
					rowDiff = false;
				}
				if (GetColumn(GetFrom(legalMoves[i])) == GetColumn(from))
				{
					columnDiff = false;
				}
			}
		}

		if (dupe)
		{
			if (columnDiff)
			{
				result += GetSquareSAN(from)[0];
			}
			else if (rowDiff)
			{
				result += GetSquareSAN(from)[1];
			}
			else
			{
				result += GetSquareSAN(from);
			}
		}
		else if (fromPieceType == PAWN && position.Board[to] != PIECE_NONE)
		{
			// Pawn captures need a row
			result += GetSquareSAN(from)[0];
		}
		
		// Capture?
		if (position.Board[to] != PIECE_NONE ||
			moveType == MoveTypeEnPassent)
		{
			result += "x";
		}

		// Target square
		result += GetSquareSAN(to);
	}

	if (moveType == MoveTypePromotion)
	{
		switch (GetPromotionMoveType(move))
		{
		case KNIGHT: result += "=N"; break;
		case BISHOP: result += "=B"; break;
		case ROOK: result += "=R"; break;
		case QUEEN: result += "=Q"; break;
		}
	}

	MoveUndo moveUndo;
	position.MakeMove(move, moveUndo);

	if (position.IsInCheck())
	{
		Move checkEscapes[64];
		result += GenerateCheckEscapeMoves(position, checkEscapes) == 0 ? "#" : "+";
	}

	position.UnmakeMove(move, moveUndo);

	return result;
}