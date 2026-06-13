// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later


namespace acus::error {

  enum class ErrorCode {
    UnexpectedApiCall,
    EmptyProgram,
    DuplicateFunctionParameters,
    DuplicateGlobalReferences,
    FunctionDoesNotExist,
    LabelDoesNotExist,
    TakingAddressOfTemporary,
    ReadOnlyExpression,
    IncompatibleOperands,
    NotPrintable,
    EntryFunctionNotDefined,
    WrongEntryFunctionType,
    ExecutionPathWithoutReturn,
    UnreachableCodeSection,
    
    ExpectedOutsideProgramBlock,
    ExpectedInsideProgramBlock,
    ExpectedOutsideFunctionBlock,
    ExpectedInsideFunctionBlock,
    ExpectedNoScope,

    GlobalDeclarationNotAllowed,
    GlobalNameUnavailable,
    ExpectedGlobal,

    NameNotInScope,
    NameAlreadyInScope,
    NameAlreadyInCurrentScope,

    ExpectedInteger,
    ExpectedSignedInteger,
    ExpectedUnsignedInteger,
    ExpectedArray,
    ExpectedString,
    ExpectedArrayOrString,
    ExpectedStruct,
    ExpectedPointer,
    ExpectedFunctionType,
    ExpectedFunctionPointer,

    FieldIndexOutOfBounds,
    FieldCountMismatch,
    MissingField,
    MultipleInitializationsOfSameField,    
    FieldNotFound,
    IndexOutOfBounds,
    NegativeIndex,
    TooManyElementsInArrayInitialization,
    TooFewElementsInArrayInitialization,
    
    AssignmentTypeMismatch,
    UnexpectedType,
    FunctionParameterCountMismatch,
    InvalidFunctionPointerCall,    
    InvalidReadType,
    
    BuilderNotFinalized,
    BuilderUsedAfterFinalize,    
    ReturnTypeSpecifiedMultipleTimes    
  };
  
} // namespace acus::error
