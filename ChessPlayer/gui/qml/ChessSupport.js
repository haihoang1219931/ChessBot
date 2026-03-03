var NOPIECE = 0;
var WHITE = 1;
var BLACK = 2;

var W_KING = 3;
var W_QUEEN = 4;
var W_ROOK = 5;
var W_BISHOP = 6;
var W_KNIGHT = 7;
var W_PAWN = 8;

var B_KING = 9;
var B_QUEEN = 10;
var B_ROOK = 11;
var B_BISHOP = 12;
var B_KNIGHT = 13;
var B_PAWN = 14;


function createChessBoardModel() {
    var chessBoardModel = [];
    for(var row =0; row <8; row ++) {
        for(var col =0; col <8; col ++){
            chessBoardModel[row*8+col] = {row:row,col:col,suggest:false,clicked:false}
        }
    }
    return chessBoardModel;
}
function createChessPiecesModel(){
    var chessPieceModel = [
        W_ROOK,  W_KNIGHT, W_BISHOP, W_QUEEN ,W_KING , W_BISHOP, W_KNIGHT, W_ROOK,
        W_PAWN,  W_PAWN,   W_PAWN,   W_PAWN,  W_PAWN,  W_PAWN,   W_PAWN,   W_PAWN,
        NOPIECE, NOPIECE,  NOPIECE,  NOPIECE, NOPIECE, NOPIECE,  NOPIECE,  NOPIECE,
        NOPIECE, NOPIECE,  NOPIECE,  NOPIECE, NOPIECE, NOPIECE,  NOPIECE,  NOPIECE,
        NOPIECE, NOPIECE,  NOPIECE,  NOPIECE, NOPIECE, NOPIECE,  NOPIECE,  NOPIECE,
        NOPIECE, NOPIECE,  NOPIECE,  NOPIECE, NOPIECE, NOPIECE,  NOPIECE,  NOPIECE,
        B_PAWN,  B_PAWN,   B_PAWN,   B_PAWN,  B_PAWN,  B_PAWN,   B_PAWN,   B_PAWN,
        B_ROOK,  B_KNIGHT, B_BISHOP, B_QUEEN ,B_KING , B_BISHOP, B_KNIGHT, B_ROOK,
    ];
    chessPieceModel[2*8+2] = B_ROOK
    chessPieceModel[2*8+3] = B_ROOK
    chessPieceModel[3*8+4] = B_ROOK
    chessPieceModel[3*8+5] = W_ROOK
    return chessPieceModel;
}

function updateSugguestMoves() {
    var possibleMoves = ChessSupport.validMove(repeaterChessPiece.model,rowClicked,colClicked);
    var arrayLength = possibleMoves.length;
    for(var i = 0; i < arrayLength; i++) {
        chessBoardModel[possibleMoves[i].row*8+possibleMoves[i].col] = possibleMoves[i];
    }
}

function showSugguestMoves() {
    var possibleMoves = ChessSupport.validMove(repeaterChessPiece.model,rowClicked,colClicked);
    var arrayLength = possibleMoves.length;
    for(var i = 0; i < arrayLength; i++) {
        chessBoardModel[possibleMoves[i].row*8+possibleMoves[i].col] = possibleMoves[i];
    }
}

function updateChessPieceModel(chessPieceModel,chessBoardModel, targetRow, targetCol) {
    var chessPiece = NOPIECE;
    for(var row =0; row <8; row ++) {
        for(var col =0; col <8; col ++){
            if(chessBoardModel[row*8+col].clicked) {
                chessPiece = chessPieceModel[row*8+col];
                chessPieceModel[row*8+col] = NOPIECE;
            } else if(chessBoardModel[row*8+col].suggest) {
                chessBoardModel[row*8+col].suggest = false;
            }
        }
    }
    console.log("updateChessPieceModel "+ targetRow +":"+targetCol + " = " + chessPiece);
    chessPieceModel[targetRow*8+targetCol] = chessPiece;
    return {board:chessBoardModel,piece:chessPieceModel};
}
function chessSide(chessPiece) {
    if(chessPiece >= W_KING && chessPiece <= W_PAWN) {
        return WHITE;
    } else if(chessPiece >= B_KING && chessPiece <= B_PAWN) {
        return BLACK;
    } else {
        return NOPIECE;
    }
}
function charCode(chessPiece) {
    if(chessPiece >= W_KING && chessPiece <= W_PAWN) {
        return String.fromCharCode(9811 + 0 * 6 + chessPiece-W_KING+1);
    } else if(chessPiece >= B_KING && chessPiece <= B_PAWN) {
        return String.fromCharCode(9811 + 1 * 6 + chessPiece-B_KING+1);
    } else {
        return "";
    }
}
function isOppositeSide(firstPiece,secondPiece) {
    var firtSide = chessSide(firstPiece);
    var secondSide = chessSide(secondPiece);
    if(firtSide === NOPIECE || secondSide === NOPIECE) {
        return false;
    } else return firtSide !== secondSide;
}
function validMove(chessPieceModel,_row,_col) {
    var possibleMoves=[];
    var chessPiece = chessPieceModel[_row*8+_col]
//    console.log("chessPiece "+)
    switch(chessPiece) {
    case W_PAWN: {
        if(chessSide(chessPieceModel[(_row+1)*8+_col])=== NOPIECE)
            possibleMoves.push({row:_row+1,col:_col,suggest:true,clicked:false});
        if(chessSide(chessPieceModel[(_row+2)*8+_col])=== NOPIECE && _row === 1)
            possibleMoves.push({row:_row+2,col:_col,suggest:true,clicked:false});
        if(_col-1 >= 0 && chessSide(chessPieceModel[(_row+1)*8+_col-1])=== BLACK)
            possibleMoves.push({row:_row+1,col:_col-1,suggest:true,clicked:false});
        if(_col+1 < 8 && chessSide(chessPieceModel[(_row+1)*8+_col+1])=== BLACK)
            possibleMoves.push({row:_row+1,col:_col+1,suggest:true,clicked:false});
        break;
    }
    case B_PAWN: {
        if(chessSide(chessPieceModel[(_row-1)*8+_col])=== NOPIECE)
            possibleMoves.push({row:_row-1,col:_col,suggest:true,clicked:false});
        if(chessSide(chessPieceModel[(_row-2)*8+_col])=== NOPIECE && _row === 6)
            possibleMoves.push({row:_row-2,col:_col,suggest:true,clicked:false});
        if(_col-1 >= 0 && chessSide(chessPieceModel[(_row-1)*8+_col-1])=== BLACK)
            possibleMoves.push({row:_row-1,col:_col-1,suggest:true,clicked:false});
        if(_col+1 < 8 && chessSide(chessPieceModel[(_row-1)*8+_col+1])=== BLACK)
            possibleMoves.push({row:_row-1,col:_col+1,suggest:true,clicked:false});
        break;
    }
    }
    return possibleMoves;
}

