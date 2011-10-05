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
	MoveSorter(const Position &position, const SearchInfo &searchInfo) : position(position), searchInfo(searchInfo)
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
		moveCount = GenerateCaptureMoves(position, moves, moveScores);
		moves[moveCount] = 0; // Sentinel move
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
        
		// Disabled since we don't order our checking moves.
		/*
         for (int i = 0; i < moveCount; i++)
         {
         moveScores[i] = 0;
         }
         */
	}
    
	inline void InitializeNormalMoves(const Move hashMove, const Move killer1, const Move killer2, const bool generatePawnMoves)
	{
		at = 0;
		state = hashMove == 0 ? MoveGenerationState_GenerateWinningEqualCaptures : MoveGenerationState_Hash;
        
        this->generatePawnMoves = generatePawnMoves;
        
		this->hashMove = hashMove;
		this->killer1 = killer1;
		this->killer2 = killer2;
	}
    
	inline Move PickBestMove()
	{
		// We use a trick here instead of checking for at == moveCount.
		// The move initialization code stores a sentinel move at the end of the movelist.
		ASSERT(at <= moveCount);
        
		s16 bestScore = moveScores[at], bestMove = moves[at];
		for (int i = ++at; i < moveCount; i++)
		{
			ASSERT(moveScores[i] >= MinEval && moveScores[i] <= MaxEval);
            
			const s16 score = moveScores[i];
			if (score > bestScore)
			{
				const Move move = moves[i];
				moves[i] = bestMove;
				moveScores[i] = bestScore;
				bestMove = move;
				bestScore = score;
			}
		}
        
		ASSERT(at >= moveCount || (moveScores[at - 1] >= moveScores[at]));
        
		return bestMove;
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
                    
                    if (bestMove == hashMove)
                    {
                        continue;
                    }
                    
                    // Losing captures go off to the back of the list, winning/equal get returned.  They will
                    // have been scored correctly already.
                    if (FastSee(position, bestMove, position.ToMove))
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
                if (killer1 != hashMove && 
                    IsMovePseudoLegal(position, killer1) &&
                    position.Board[GetTo(killer1)] == PIECE_NONE)
                {
                    state = MoveGenerationState_Killer2;
                    return killer1;
                }
                
                // Intentional fall-through
                
            case MoveGenerationState_Killer2:
                if (killer2 != hashMove &&
                    IsMovePseudoLegal(position, killer2) &&
                    position.Board[GetTo(killer2)] == PIECE_NONE)
                {
                    state = MoveGenerationState_GenerateQuietMoves;
                    return killer2;
                }
                
                // Intentional fall-through
                
            case MoveGenerationState_GenerateQuietMoves:
                if (generatePawnMoves)
                {
                    moveCount = GenerateQuietMoves(position, moves);
                }
                else
                {
                    moveCount = GenerateSliderMoves(position, moves);
                }
                at = 0;
                
                for (int i = 0; i < moveCount; i++)
                {
                    int historyScore = searchInfo.History[position.Board[GetFrom(moves[i])]][GetTo(moves[i])];
                    if (historyScore > 32767) historyScore = 32767;
                    moveScores[i] = historyScore;
                }
                
                state = MoveGenerationState_QuietMoves;
                // Intentional fall-through
                
            case MoveGenerationState_QuietMoves:
                while (at < moveCount)
                {
                    const Move bestMove = PickBestMove();
                    if (bestMove == hashMove || bestMove == killer1 || bestMove == killer2)
                    {
                        continue;
                    }
                    return bestMove;
                }
                
                // Intentional fall-through
                
            case MoveGenerationState_GenerateLosingCaptures:
                at = 0;
                state = MoveGenerationState_LosingCaptures;
                
                // Intentional fall-through
                
            case MoveGenerationState_LosingCaptures:
                while (at < losingCapturesCount)
                {
                    const Move bestMove = losingCaptures[at++];
                    if (bestMove == hashMove)
                    {
                        continue;
                    }
                    return bestMove;
                }
                break;
                
            case MoveGenerationState_CheckEscapes:
                return PickBestMove();
		}
        
		return 0;
	}
    
private:
    
	Move moves[maxMoves];
	s16 moveScores[maxMoves];
	int moveCount;
    
	Move losingCaptures[32];
	int losingCapturesCount;
    
    bool generatePawnMoves;
	int at;
	const Position &position;
    const SearchInfo &searchInfo;
	MoveGenerationState state;
	Move hashMove, killer1, killer2;
};
