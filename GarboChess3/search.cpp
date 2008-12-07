#include "garbochess.h"
#include "position.h"
#include "movegen.h"
#include "search.h"
#include "evaluation.h"

template<class T>
void Swap(T& a, T &b)
{
	T tmp = a;
	a = b;
	b = tmp;
}

// This version of SEE does not calculate the exact material imbalance, it just returns true = winning or equal, false = losing
bool FastSee(const Position &position, const Move move)
{
	// TODO: Use PSQ tables in SEE?
	const int pieceValues[8] = { 0, 1, 3, 3, 5, 10, 10000, 0 };

	const Square from = GetFrom(move);
	const Square to = GetTo(move);

	const Color us = position.ToMove;
	const Color them = FlipColor(us);

	ASSERT(position.Board[from] != PIECE_NONE);
	ASSERT(GetPieceColor(position.Board[from]) == us);
	ASSERT(position.Board[to] == PIECE_NONE || GetPieceColor(position.Board[from]) != GetPieceColor(position.Board[to]));

	const int fromValue = pieceValues[GetPieceType(position.Board[from])];
	const int toValue = pieceValues[GetPieceType(position.Board[to])];

	if (fromValue <= toValue)
	{
		return true;
	}

	if (GetMoveType(move) == MoveTypeEnPassent)
	{
		// e.p. captures are always pxp which is winning or equal.
		return true;
	}

	// We should only be making initially losing captures here
	ASSERT(fromValue > pieceValues[PAWN] || GetMoveType(move) == MoveTypePromotion);

	const Bitboard enemyPieces = position.Colors[them];

	// Pawn attacks
	// If any opponent pawns can capture back, this capture is not worthwhile.
	// This is because any pawn captures will return true above, so we must be using a knight or above.
	if (GetPawnAttacks(to, us) & position.Pieces[PAWN] & enemyPieces)
	{
		return false;
	}

	// Knight attacks
	Bitboard attackingPieces = GetKnightAttacks(to) & position.Pieces[KNIGHT];
	const int captureDeficit = fromValue - toValue;
	// If any opponent knights can capture back, and the deficit we have to make up is greater than the knights value,
	// it's not worth it.  We can capture on this square again, and the opponent doesn't have to capture back.
	if (captureDeficit > pieceValues[KNIGHT] && (attackingPieces & enemyPieces))
	{
		return false;
	}

	// Bishop attacks
	Bitboard allPieces = position.Colors[us] | enemyPieces;
	XorClearBit(allPieces, from);

	attackingPieces |= GetBishopAttacks(to, allPieces) & (position.Pieces[BISHOP] | position.Pieces[QUEEN]);
	if (captureDeficit > pieceValues[BISHOP] && (attackingPieces & position.Pieces[BISHOP] & enemyPieces))
	{
		return false;
	}

	// Pawn defenses
	// At this point, we are sure we are making a "losing" capture.  The opponent can not capture back with a
	// pawn.  They cannot capture back with a bishop/knight and stand pat either.  So, if we can capture with
	// a pawn, it's got to be a winning or equal capture.
	if (GetPawnAttacks(to, them) & position.Pieces[PAWN] & position.Colors[us])
	{
		return true;
	}

	// Rook attacks
	attackingPieces |= GetRookAttacks(to, allPieces) & (position.Pieces[ROOK] | position.Pieces[QUEEN]);
	if (captureDeficit > pieceValues[ROOK] && (attackingPieces & position.Pieces[ROOK] & enemyPieces))
	{
		return false;
	}

	// King attacks
	attackingPieces |= GetKingAttacks(to) & position.Pieces[KING];

	// Make sure our original attacking piece is not included in the set of attacking pieces
	attackingPieces &= allPieces;

	if (!attackingPieces)
	{
		// No pieces attacking us, we have won
		return true;
	}

	// At this point, we have built a bitboard of all the pieces attacking the "to" square.  This includes
	// potential x-ray attacks from removing the "from" square piece, which we used to do the capture.

	// We are currently winning the amount of material of the captured piece, time to see if the opponent
	// can get it back somehow.  We assume the opponent can capture our current piece in this score, which
	// simplifies the later code considerably.
	int seeValue = toValue - fromValue;

	for (;;)
	{
		int capturingPieceValue = -1;

		// Find the least valuable piece of the opponent that can attack the square
		for (PieceType capturingPiece = KNIGHT; capturingPiece <= KING; capturingPiece++)
		{
			const Bitboard attacks = position.Pieces[capturingPiece] & attackingPieces & position.Colors[them];
			if (attacks)
			{
				// They can capture with a piece
				capturingPieceValue = pieceValues[capturingPiece];
				
				const Square attackingSquare = GetFirstBitIndex(attacks);
				XorClearBit(attackingPieces, attackingSquare);
				XorClearBit(allPieces, attackingSquare);
				break;
			}
		}

		if (capturingPieceValue == -1)
		{
			// The opponent can't capture back - we win
			return true;
		}

		// Now, if seeValue < 0, the opponent is winning.  If even after we take their piece,
		// we can't bring it back to 0, then we have lost this battle.
		seeValue += capturingPieceValue;
		if (seeValue < 0)
		{
			return false;
		}

		// Add any x-ray attackers
		attackingPieces |= 
			((GetBishopAttacks(to, allPieces) & (position.Pieces[BISHOP] | position.Pieces[QUEEN])) |
			(GetRookAttacks(to, allPieces) & (position.Pieces[ROOK] | position.Pieces[QUEEN]))) & allPieces;

		// Our turn to capture
		capturingPieceValue = -1;

		// Find the least valuable piece of the opponent that can attack the square
		for (PieceType capturingPiece = KNIGHT; capturingPiece <= KING; capturingPiece++)
		{
			const Bitboard attacks = position.Pieces[capturingPiece] & attackingPieces & position.Colors[us];
			if (attacks)
			{
				// We can capture with a piece
				capturingPieceValue = pieceValues[capturingPiece];
				
				const Square attackingSquare = GetFirstBitIndex(attacks);
				XorClearBit(attackingPieces, attackingSquare);
				XorClearBit(allPieces, attackingSquare);
				break;
			}
		}

		if (capturingPieceValue == -1)
		{
			// We can't capture back, we lose :(
			return false;
		}

		// Assume our opponent can capture us back, and if we are still winning, we can stand-pat
		// here, and assume we've won.
		seeValue -= capturingPieceValue;
		if (seeValue >= 0)
		{
			return true;
		}

		// Add any x-ray attackers
		attackingPieces |= 
			((GetBishopAttacks(to, allPieces) & (position.Pieces[BISHOP] | position.Pieces[QUEEN])) |
			(GetRookAttacks(to, allPieces) & (position.Pieces[ROOK] | position.Pieces[QUEEN]))) & allPieces;

	}
}

template<int maxMoves>
class MoveSorter
{
public:
	inline int GetMoveCount() const
	{
		return moveCount;
	}

	void GenerateQCapture(const Position &position)
	{
		moveCount = GenerateCaptureMoves(position, moves);
		moves[moveCount] = 0; // Sentinel move

		for (int i = 0; i < moveCount; i++)
		{
			const PieceType fromPiece = GetPieceType(position.Board[GetFrom(moves[i])]);
			const PieceType toPiece = GetPieceType(position.Board[GetTo(moves[i])]);

			moveScores[i] = ScoreCaptureMove(moves[i], fromPiece, toPiece);
		}
	}

	void GenerateCheckEscape(const Position &position)
	{
		moveCount = GenerateCheckEscapeMoves(position, moves);
		moves[moveCount] = 0;

		for (int i = 0; i < moveCount; i++)
		{
			const PieceType fromPiece = GetPieceType(position.Board[GetFrom(moves[i])]);
			const PieceType toPiece = GetPieceType(position.Board[GetTo(moves[i])]);

			moveScores[i] = toPiece != PIECE_NONE ? 
				// Sort captures first
				ScoreCaptureMove(moves[i], fromPiece, toPiece) :
				// Score non-captures as equal
				0;
		}
	}

	inline Move NextMove()
	{
		// We use a trick here instead of checking for at == moveCount.
		// The move initialization code stores a sentinel move at the end of the movelist.
		ASSERT(at <= moveCount);

		for (int i = at + 1; i < moveCount; i++)
		{
			if (moveScores[i] > moveScores[at])
			{
				Swap(moveScores[at], moveScores[i]);
				Swap(moves[at], moves[i]);
			}
		}

		ASSERT((at + 1 >= moveCount) || (moveScores[at] >= moveScores[at + 1]));

		return moves[at++];
	}

private:

	int ScoreCaptureMove(const Move move, const PieceType fromPiece, const PieceType toPiece)
	{
		// Currently scoring moves using MVV/LVA scoring.  ie. PxQ goes first, BxQ next, ... , KxP last
		const Move moveType = GetMoveType(move);
		if (moveType == MoveTypeNone)
		{
			ASSERT(fromPiece != PIECE_NONE);
			ASSERT(toPiece != PIECE_NONE);

			return toPiece * 100 - fromPiece;
		}
		else
		{
			if (moveType == MoveTypeEnPassent)
			{
				// e.p. captures
				return PAWN * 100 - PAWN;
			}
			else
			{
				// Queen promotions
				ASSERT(moveType == MoveTypePromotion);
				ASSERT(GetPromotionMoveType(move) == QUEEN);

				if (toPiece == PIECE_NONE)
				{
					// Goes first
					return QUEEN * 100;
				}
				else
				{
					return toPiece * 100 - QUEEN;
				}
			}
		}
	}

	Move moves[maxMoves];
	int moveScores[maxMoves];
	int moveCount;
	int at;
};


// depth == 0 is normally what is called for q-search.
// Checks are searched when depth >= -(OnePly / 2).  Depth is decreased by 1 for checks
int QSearch(Position &position, int alpha, const int beta, const int depth)
{
	// TODO: check for draws here?

	// What do we want from our evaluation? - this needs to be decided (mobility/threat information?)
	int eval = Evaluate(position);

	if (eval > alpha)
	{
		alpha = eval;
		if (alpha >= beta)
		{
			return eval;
		}
	}
	else
	{
		// TODO: Don't even try captures that won't make it back up to alpha?
	}
	
	MoveSorter<64> moves;
	moves.GenerateQCapture(position);

	Move move;
	while ((move = moves.NextMove()) != 0)
	{
		if (!FastSee(position, move))
		{
			// TODO: use SEE when depth >= 0? - tapered q-search ideas
			continue;
		}

		MoveUndo moveUndo;
		position.MakeMove(move, moveUndo);

		if (!position.CanCaptureKing())
		{
			int value;

			// Search this move
			if (position.IsCheck())
			{
				value = -QSearchCheck(position, -beta, -alpha, depth - 1);
			}
			else
			{
				value = -QSearch(position, -beta, -alpha, depth - OnePly);
			}

			if (value > eval)
			{
				eval = value;
				if (value > alpha)
				{
					alpha = value;
					if (value > beta)
					{
						return value;
					}
				}
			}
		}

		position.UnmakeMove(move, moveUndo);
	}

	if (depth < -(OnePly / 2))
	{
		// Don't bother searching checking moves
		return eval;
	}

	// TODO: Generate and search quiet checks
	return eval;
}

int QSearchCheck(Position &position, int alpha, const int beta, const int depth)
{
	ASSERT(position.IsCheck());

	// TODO: check for draws here

	MoveSorter<64> moves;
	moves.GenerateCheckEscape(position);

	// TODO: singular move extension?

	int bestScore = MinEval;

	Move move;
	while ((move = moves.NextMove()) != 0)
	{
		MoveUndo moveUndo;
		position.MakeMove(move, moveUndo);

		ASSERT(!position.CanCaptureKing());

		int value;
		if (position.IsCheck())
		{
			value = -QSearchCheck(position, -beta, -alpha, depth - 1);
		}
		else
		{
			value = -QSearch(position, -beta, -alpha, depth - OnePly);
		}

		if (value > bestScore)
		{
			bestScore = value;
			if (value > alpha)
			{
				alpha = value;
				if (value > beta)
				{
					return value;
				}
			}
		}

		position.UnmakeMove(move, moveUndo);
	}

	return bestScore;
}

// TODO: investigate using root move sorting in PV nodes (need PV hash table)
// TODO: investigate heavy pruning of the search tree (>500cp eval = pruning time)