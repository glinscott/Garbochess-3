#include "garbochess.h"
#include "position.h"
#include "movegen.h"
#include "search.h"
#include "evaluation.h"
#include "hashtable.h"

#include <cstdlib>

template<class T>
void Swap(T& a, T &b)
{
	T tmp = a;
	a = b;
	b = tmp;
}

const int MoveSentinelScore = MinEval - 1;
const int MaxPly = 99;
const int MaxThreads = 8;

// TODO: make this use contempt?
const int DrawScore = 0;

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
	ASSERT(fromValue > pieceValues[PAWN] || GetMoveType(move) == MoveTypePromotion || toValue == 0);

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

void StableSortMoves(Move *moves, int *moveScores, int moveCount)
{
	// Stable sort the moves
	for (int i = 1; i < moveCount; i++)
	{
		int value = moveScores[i];
		Move move = moves[i];

		int j = i - 1;
		for (; j >= 0 && moveScores[j] < value; j--)
		{
			moveScores[j + 1] = moveScores[j];
			moves[j + 1] = moves[j];
		}

		moveScores[j + 1] = value;
		moves[j + 1] = move;
	}
}

enum MoveGenerationState
{
	MoveGenerationState_Hash,
	MoveGenerationState_GenerateWinningEqualCaptures,
	MoveGenerationState_WinningEqualCaptures,
	MoveGenerationState_Killer1,
	MoveGenerationState_Killer2,
	MoveGenerationState_GenerateQuietMoves,
	MoveGenerationState_QuietMoves,
	MoveGenerationState_GenerateLosingCaptures,
	MoveGenerationState_LosingCaptures,
	MoveGenerationState_CheckEscapes,
};

template<int maxMoves>
class MoveSorter
{
public:
	MoveSorter(const Position &position) : position(position)
	{
	}

	inline int GetMoveCount() const
	{
		return moveCount;
	}

	inline MoveGenerationState GetMoveGenerationState() const
	{
		return state;
	}

	inline void GenerateCaptures()
	{
		at = 0;
		moveCount = GenerateCaptureMoves(position, moves);
		moves[moveCount] = 0; // Sentinel move

		for (int i = 0; i < moveCount; i++)
		{
			const PieceType fromPiece = GetPieceType(position.Board[GetFrom(moves[i])]);
			const PieceType toPiece = GetPieceType(position.Board[GetTo(moves[i])]);

			moveScores[i] = ScoreCaptureMove(moves[i], fromPiece, toPiece);
		}
	}

	inline void GenerateCheckEscape()
	{
		at = 0;
		moveCount = GenerateCheckEscapeMoves(position, moves);
		moves[moveCount] = 0;

		state = MoveGenerationState_CheckEscapes;

		for (int i = 0; i < moveCount; i++)
		{
			const PieceType toPiece = GetPieceType(position.Board[GetTo(moves[i])]);

			moveScores[i] = toPiece != PIECE_NONE ? 
				// Sort captures first
				ScoreCaptureMove(moves[i], GetPieceType(position.Board[GetFrom(moves[i])]), toPiece) :
				// Score non-captures as equal
				0;
		}
	}

	inline void GenerateChecks()
	{
		at = 0;
		moveCount = GenerateCheckingMoves(position, moves);
		moves[moveCount] = 0;

		for (int i = 0; i < moveCount; i++)
		{
			moveScores[i] = 0;
		}
	}

	inline void InitializeNormalMoves(const Move hashMove, const Move killer1, const Move killer2)
	{
		at = 0;
		state = hashMove == 0 ? MoveGenerationState_GenerateWinningEqualCaptures : MoveGenerationState_Hash;

		this->hashMove = hashMove;
		this->killer1 = killer1;
		this->killer2 = killer2;
	}

	inline Move PickBestMove()
	{
		// We use a trick here instead of checking for at == moveCount.
		// The move initialization code stores a sentinel move at the end of the movelist.
		ASSERT(at <= moveCount);

		int bestScore = moveScores[at], bestIndex = at;
		for (int i = at + 1; i < moveCount; i++)
		{
			ASSERT(moveScores[i] >= MinEval && moveScores[i] <= MaxEval);

			if (moveScores[i] > bestScore)
			{
				bestIndex = i;
				bestScore = moveScores[i];
			}
		}

		if (bestIndex != at)
		{
			Swap(moveScores[at], moveScores[bestIndex]);
			Swap(moves[at], moves[bestIndex]);
		}

		ASSERT((at + 1 >= moveCount) || (moveScores[at] >= moveScores[at + 1]));

		return moves[at++];
	}

	inline Move NextQMove()
	{
		return PickBestMove();
	}

	inline Move NextNormalMove()
	{
		ASSERT(state >= MoveGenerationState_Hash && state <= MoveGenerationState_CheckEscapes);

		switch (state)
		{
		case MoveGenerationState_Hash:
			if (IsMovePseudoLegal(position, hashMove))
			{
				state = MoveGenerationState_GenerateWinningEqualCaptures;
				return hashMove;
			}

			// Intentional fall-through

		case MoveGenerationState_GenerateWinningEqualCaptures:
			GenerateCaptures();
			losingCapturesCount = 0;

			state = MoveGenerationState_WinningEqualCaptures;

			// Intentional fall-through

		case MoveGenerationState_WinningEqualCaptures:
			while (at < moveCount)
			{
				const Move bestMove = PickBestMove();
				ASSERT(bestMove != 0);

				// Losing captures go off to the back of the list, winning/equal get returned.  They will
				// have been scored correctly already.
				if (FastSee(position, bestMove))
				{
					return bestMove;
				}
				else
				{
					losingCaptures[losingCapturesCount++] = bestMove;
				}
			}

			// Intentional fall-through

		case MoveGenerationState_Killer1:
			if (IsMovePseudoLegal(position, killer1))
			{
				state = MoveGenerationState_Killer2;
				return killer1;
			}

			// Intentional fall-through

		case MoveGenerationState_Killer2:
			if (IsMovePseudoLegal(position, killer2))
			{
				state = MoveGenerationState_GenerateQuietMoves;
				return killer2;
			}
			
			// Intentional fall-through

		case MoveGenerationState_GenerateQuietMoves:
			moveCount = GenerateQuietMoves(position, moves);
			at = 0;

			for (int i = 0; i < moveCount; i++)
			{
				// TODO: history?
				moveScores[i] = 0;
			}

			state = MoveGenerationState_QuietMoves;
			// Intentional fall-through

		case MoveGenerationState_QuietMoves:
			while (at < moveCount)
			{
				return PickBestMove();
			}

			// Intentional fall-through

		case MoveGenerationState_GenerateLosingCaptures:
			at = 0;
			state = MoveGenerationState_LosingCaptures;

			// Intentional fall-through

		case MoveGenerationState_LosingCaptures:
			while (at < losingCapturesCount)
			{
				return losingCaptures[at++];
			}
			break;

		case MoveGenerationState_CheckEscapes:
			return PickBestMove();
		}

		return 0;
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
				// Promotions
				ASSERT(moveType == MoveTypePromotion);

				if (GetPromotionMoveType(move) == QUEEN)
				{
					// Goes first
					return QUEEN * 100;
				}
				else
				{
					// Other types of promotions, we don't really care about
					return 0;
				}
			}
		}
	}

	Move moves[maxMoves];
	int moveScores[maxMoves];
	int moveCount;

	Move losingCaptures[32];
	int losingCapturesCount;

	int at;
	const Position &position;
	MoveGenerationState state;
	Move hashMove, killer1, killer2;
};

struct SearchInfo
{
	int NodeCount;
	int QNodeCount;

	Move Killers[MaxPly][2];
};

const int SearchInfoPageSize = 4096;
u64 searchInfoThreads;

SearchInfo &GetSearchInfo(int thread)
{
	return (SearchInfo&)*((SearchInfo*)(searchInfoThreads + (SearchInfoPageSize * thread)));
}

// depth == 0 is normally what is called for q-search.
// Checks are searched when depth >= -(OnePly / 2).  Depth is decreased by 1 for checks
int QSearch(Position &position, int alpha, const int beta, const int depth)
{
	ASSERT(!position.IsInCheck());

	if (position.IsDraw())
	{
		return DrawScore;
	}

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
	
	MoveSorter<64> moves(position);
	moves.GenerateCaptures();

	Move move;
	while ((move = moves.NextQMove()) != 0)
	{
		if (!FastSee(position, move))
		{
			// TODO: we are excluding losing checks here as well - is this safe?
			// TODO: use SEE when depth >= 0? - tapered q-search ideas
			continue;
		}

		MoveUndo moveUndo;
		position.MakeMove(move, moveUndo);

		if (!position.CanCaptureKing())
		{
			int value;

			// Search this move
			if (position.IsInCheck())
			{
				value = -QSearchCheck(position, -beta, -alpha, depth - 1);
			}
			else
			{
				value = -QSearch(position, -beta, -alpha, depth - OnePly);
			}

			position.UnmakeMove(move, moveUndo);

			if (value > eval)
			{
				eval = value;
				if (value > alpha)
				{
					alpha = value;
					if (value >= beta)
					{
						return value;
					}
				}
			}
		}
		else
		{
			position.UnmakeMove(move, moveUndo);
		}
	}

	if (depth < -(OnePly / 2))
	{
		// Don't bother searching checking moves
		return eval;
	}

	moves.GenerateChecks();

	while ((move = moves.NextQMove()) != 0)
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
			ASSERT(position.IsInCheck());

			int value = -QSearchCheck(position, -beta, -alpha, depth - 1);

			position.UnmakeMove(move, moveUndo);
			
			if (value > eval)
			{
				eval = value;
				if (value > alpha)
				{
					alpha = value;
					if (value >= beta)
					{
						return beta;
					}
				}
			}
		}
		else
		{
			position.UnmakeMove(move, moveUndo);
		}
	}
	
	return eval;
}

int QSearchCheck(Position &position, int alpha, const int beta, const int depth)
{
	ASSERT(position.IsInCheck());

	if (position.IsDraw())
	{
		return DrawScore;
	}

	MoveSorter<64> moves(position);
	moves.GenerateCheckEscape();

	if (moves.GetMoveCount() == 0)
	{
		// Checkmate
		return MinEval;
	}

	int bestScore = MoveSentinelScore;

	// Single-reply to check extension
	const int depthReduction = moves.GetMoveCount() == 1 ? 1 : OnePly;

	Move move;
	while ((move = moves.NextQMove()) != 0)
	{
		MoveUndo moveUndo;
		position.MakeMove(move, moveUndo);

		ASSERT(!position.CanCaptureKing());

		int value;
		if (position.IsInCheck())
		{
			value = -QSearchCheck(position, -beta, -alpha, depth - 1);
		}
		else
		{
			value = -QSearch(position, -beta, -alpha, depth - depthReduction);
		}

		position.UnmakeMove(move, moveUndo);

		if (value > bestScore)
		{
			bestScore = value;
			if (value > alpha)
			{
				alpha = value;
				if (value >= beta)
				{
					return value;
				}
			}
		}
	}

	return bestScore;
}

int Search(Position &position, SearchInfo &searchInfo, const int beta, const int depth, const bool inCheck)
{
	ASSERT(depth > 0);
	ASSERT(inCheck ? position.IsInCheck() : !position.IsInCheck());

	if (position.IsDraw())
	{
		return DrawScore;
	}

	HashEntry *hashEntry;
	Move hashMove;
	if (ProbeHash(position.Hash, hashEntry))
	{
		if (hashEntry->Depth >= depth)
		{
			const int hashFlags = hashEntry->GetHashFlags();
			if (hashFlags == HashFlagsExact)
			{
				return hashEntry->Score;
			}
			else if (hashEntry->Score >= beta && hashFlags == HashFlagsBeta)
			{
				return hashEntry->Score;
			}
			else if (hashEntry->Score < beta && hashFlags == HashFlagsAlpha)
			{
				return hashEntry->Score;
			}
		}

		hashMove = hashEntry->Move;
	}
	else
	{
		hashMove = 0;
	}
	
	if (!inCheck)
	{
		// TODO: null move should not happen in endgame situations...  low material being the main one.

		// Attempt to null-move
		MoveUndo moveUndo;
		position.MakeNullMove(moveUndo);

		const int newDepth = depth - (4 * OnePly);
		int score;
		if (newDepth <= 0)
		{
			score = -QSearch(position, -beta, 1 - beta, 0);
		}
		else
		{
			// TODO: we should not allow recursive null-moves.
			score = -Search(position, searchInfo, 1 - beta, newDepth, false);
		}

		position.UnmakeNullMove(moveUndo);

		if (score >= beta)
		{
			// TODO: store null move cutoffs in hash table?
			return score;
		}
	}

	MoveSorter<256> moves(position);
	bool singular = false;
	if (!inCheck)
	{
		moves.InitializeNormalMoves(hashMove, searchInfo.Killers[depth][0], searchInfo.Killers[depth][1]);
	}
	else
	{
		moves.GenerateCheckEscape();
		if (moves.GetMoveCount() == 0)
		{
			return MinEval;
		}
		else if (moves.GetMoveCount() == 1)
		{
			singular = true;
		}
	}
	
	int bestScore = MoveSentinelScore;
	Move move;
	while ((move = moves.NextNormalMove()) != 0)
	{
		MoveUndo moveUndo;
		position.MakeMove(move, moveUndo);

		if (!position.CanCaptureKing())
		{
			int value;

			// Search move
			const bool isChecking = position.IsInCheck();
			const int newDepth = (isChecking || singular) ? depth : depth - OnePly;

			if (newDepth <= 0)
			{
				 value = -QSearch(position, -beta, 1 - beta, 0);
			}
			else
			{
				value = -Search(position, searchInfo, 1 - beta, newDepth, isChecking);
			}

			position.UnmakeMove(move, moveUndo);

			if (value > bestScore)
			{
				bestScore = value;

				if (value >= beta)
				{
					// TODO: update history

					StoreHash(position.Hash, value, move, depth, HashFlagsBeta);

					// Update killers
					if (move != searchInfo.Killers[depth][0])
					{
						searchInfo.Killers[depth][1] = searchInfo.Killers[depth][0];
						searchInfo.Killers[depth][0] = move;
					}

					return value;
				}
			}
		}
		else
		{
			position.UnmakeMove(move, moveUndo);
		}
	}

	if (bestScore == MoveSentinelScore)
	{
		ASSERT(!inCheck);
		// Stalemate
		return DrawScore;
	}

	// TODO: some sort of history update here?

	StoreHash(position.Hash, bestScore, 0, depth, HashFlagsAlpha);

	return bestScore;
}

int SearchPV(Position &position, SearchInfo &searchInfo, int alpha, const int beta, const int depth, const bool inCheck)
{
	ASSERT(inCheck ? position.IsInCheck() : !position.IsInCheck());

	if (depth <= 0)
	{
		ASSERT(!inCheck);
		return QSearch(position, alpha, beta, 0);
	}

	if (position.IsDraw())
	{
		return DrawScore;
	}

	HashEntry *hashEntry;
	Move hashMove;
	if (ProbeHash(position.Hash, hashEntry))
	{
		hashMove = hashEntry->Move;
	}
	else
	{
		hashMove = 0;
	}

	// TODO: iid if no hashMove!

	// TODO: use root move sorting in PV positions?
	MoveSorter<256> moves(position);
	bool singular = false;

	if (!inCheck)
	{
		moves.InitializeNormalMoves(hashMove, searchInfo.Killers[depth][0], searchInfo.Killers[depth][1]);
	}
	else
	{
		moves.GenerateCheckEscape();
		if (moves.GetMoveCount() == 0)
		{
			return MinEval;
		}
		else if (moves.GetMoveCount() == 1)
		{
			singular = true;
		}
	}

	const int originalAlpha = alpha;
	int bestScore = MoveSentinelScore;
	hashMove = 0;

	Move move;
	while ((move = moves.NextNormalMove()) != 0)
	{
		MoveUndo moveUndo;
		position.MakeMove(move, moveUndo);

		if (!position.CanCaptureKing())
		{
			int value;

			// Search move
			const bool isChecking = position.IsInCheck();
			const int newDepth = (isChecking || singular) ? depth : depth - OnePly;

			if (bestScore == MoveSentinelScore)
			{
				value = -SearchPV(position, searchInfo, -beta, -alpha, newDepth, isChecking);
			}
			else
			{
				if (newDepth <= 0)
				{
					value = -QSearch(position, -beta, -alpha, 0);
				}
				else
				{
					value = -Search(position, searchInfo, -alpha, newDepth, isChecking);
				}

				if (value > alpha)
				{
					value = -SearchPV(position, searchInfo, -beta, -alpha, newDepth, isChecking);
				}
			}

			position.UnmakeMove(move, moveUndo);

			if (value > bestScore)
			{
				bestScore = value;
				hashMove = move;

				if (value > alpha)
				{
					alpha = value;

					if (value >= beta)
					{
						// TODO: update history
						StoreHash(position.Hash, value, move, depth, HashFlagsBeta);

						// Update killers
						if (move != searchInfo.Killers[depth][0])
						{
							searchInfo.Killers[depth][1] = searchInfo.Killers[depth][0];
							searchInfo.Killers[depth][0] = move;
						}

						return value;
					}
				}
			}
		}
		else
		{
			position.UnmakeMove(move, moveUndo);
		}
	}

	if (bestScore == MoveSentinelScore)
	{
		ASSERT(!inCheck);
		// Stalemate
		return DrawScore;
	}

	StoreHash(position.Hash, bestScore, hashMove, depth, bestScore > originalAlpha ? HashFlagsExact : HashFlagsAlpha);

	return bestScore;
}

int SearchRoot(Position &position, Move *moves, int *moveScores, int moveCount, int alpha, const int beta, const int depth)
{
	ASSERT(depth % OnePly == 0);

	int originalAlpha = alpha;
	int bestScore = MoveSentinelScore;

	for (int i = 0; i < moveCount; i++)
	{
		MoveUndo moveUndo;
		position.MakeMove(moves[i], moveUndo);

		const bool isChecking = position.IsInCheck();
		const int newDepth = isChecking ? depth : depth - OnePly;
		int value;
		if (bestScore == MoveSentinelScore)
		{
			value = -SearchPV(position, GetSearchInfo(0), -beta, -alpha, newDepth, isChecking);
		}
		else
		{
			if (newDepth <= 0)
			{
				value = -QSearch(position, -beta, -alpha, newDepth);
			}
			else
			{
				value = -Search(position, GetSearchInfo(0), -alpha, newDepth, isChecking);
			}

			// Research if value > alpha, as this means this node is a new PV node.
			if (value > alpha)
			{
				value = -SearchPV(position, GetSearchInfo(0), -beta, -alpha, newDepth, isChecking);
			}
		}

		position.UnmakeMove(moves[i], moveUndo);

		// Update move scores
		if (value <= alpha)
		{
			moveScores[i] = originalAlpha;
		}
		else if (value >= beta)
		{
			moveScores[i] = beta;
		}
		else
		{
			moveScores[i] = value;
		}

		if (value > bestScore)
		{
			bestScore = value;
			if (value > alpha)
			{
				alpha = value;
				if (value >= beta)
				{
					return value;
				}
			}
		}
	}

	return bestScore;
}

Move IterativeDeepening(Position &position, const int maxDepth, int &score)
{
	Move moves[256];
	int moveCount = GenerateLegalMoves(position, moves);
 
	// Initial sorting of root moves is done by q-search scores
	int moveScores[256];
	for (int i = 0; i < moveCount; i++)
	{
		MoveUndo moveUndo;
		position.MakeMove(moves[i], moveUndo);
		if (position.IsInCheck())
		{
			moveScores[i] = -QSearchCheck(position, MinEval, MaxEval, 0);
		}
		else
		{
			moveScores[i] = -QSearch(position, MinEval, MaxEval, 0);
		}
		position.UnmakeMove(moves[i], moveUndo);
	}

	StableSortMoves(moves, moveScores, moveCount);

	int alpha = MinEval, beta = MaxEval;

	// Iterative deepening loop
	for (int depth = 1; depth <= min(maxDepth, 99); depth++)
	{
		for (int i = 0; i < moveCount; i++)
		{
			// Reset moveScores for things we haven't searched yet
			moveScores[i] = MinEval;
		}

		int value = SearchRoot(position, moves, moveScores, moveCount, alpha, beta, depth * OnePly);

		StableSortMoves(moves, moveScores, moveCount);
	}

	// Best move is the first move in the list
	score = moveScores[0];
	return moves[0];
}

void InitializeSearch()
{
	ASSERT(sizeof(SearchInfo) < SearchInfoPageSize);

	searchInfoThreads = u64(malloc(SearchInfoPageSize * (MaxThreads + 1)));
	searchInfoThreads += SearchInfoPageSize - (searchInfoThreads & (SearchInfoPageSize - 1));
}

// TODO: check extensions limited by SEE in non-PV nodes?
// TODO: investigate using root move sorting in PV nodes (need PV hash table)
// TODO: investigate heavy pruning of the search tree (>500cp eval = pruning time)