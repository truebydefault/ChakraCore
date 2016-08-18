//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#ifdef ASMJS_PLAT
namespace AsmJsRegSlots
{
    enum ConstSlots
    {
        ReturnReg = 0,
        ModuleMemReg,
        ArrayReg,
        BufferReg,
        LengthReg,
        RegCount
    };
};

class IRBuilderAsmJs
{
    friend struct IRBuilderAsmJsSwitchAdapter;

public:
    IRBuilderAsmJs(Func * func)
        : m_func(func)
        , m_IsTJLoopBody(false)
        , m_switchAdapter(this)
        , m_switchBuilder(&m_switchAdapter)
    {
        func->m_workItem->InitializeReader(m_jnReader, m_statementReader);
        m_asmFuncInfo = m_func->GetJnFunction()->GetAsmJsFunctionInfoWithLock();
        if (func->IsLoopBody())
        {
            Js::LoopEntryPointInfo* loopEntryPointInfo = (Js::LoopEntryPointInfo*)(func->m_workItem->GetEntryPoint());
            if (loopEntryPointInfo->GetIsTJMode())
            {
                m_IsTJLoopBody = true;
                func->isTJLoopBody = true;
            }
        }
    }

    void Build();

private:

    void                    AddInstr(IR::Instr * instr, uint32 offset);
    bool                    IsLoopBody()const;
    uint                    GetLoopBodyExitInstrOffset() const;
    IR::SymOpnd *           BuildLoopBodySlotOpnd(SymID symId);
    IR::SymOpnd *           BuildAsmJsLoopBodySlotOpnd(SymID symId, IRType opndType);
    void                    EnsureLoopBodyLoadSlot(SymID symId);
    void                    EnsureLoopBodyAsmJsLoadSlot(SymID symId, IRType type);
    bool                    IsLoopBodyOuterOffset(uint offset) const;
    bool                    IsLoopBodyReturnIPInstr(IR::Instr * instr) const;
    IR::Opnd *              InsertLoopBodyReturnIPInstr(uint targetOffset, uint offset);
    IR::Instr *             CreateLoopBodyReturnIPInstr(uint targetOffset, uint offset);
    IR::RegOpnd *           BuildDstOpnd(Js::RegSlot dstRegSlot, IRType type);
    IR::RegOpnd *           BuildSrcOpnd(Js::RegSlot srcRegSlot, IRType type);
    IR::RegOpnd *           BuildIntConstOpnd(Js::RegSlot regSlot);
    SymID                   BuildSrcStackSymID(Js::RegSlot regSlot, IRType type = IRType::TyVar);

    IR::SymOpnd *           BuildFieldOpnd(Js::RegSlot reg, Js::PropertyId propertyId, PropertyKind propertyKind, IRType type, bool scale = true);
    PropertySym *           BuildFieldSym(Js::RegSlot reg, Js::PropertyId propertyId, PropertyKind propertyKind);
    uint                    AddStatementBoundary(uint statementIndex, uint offset);
    BranchReloc *           AddBranchInstr(IR::BranchInstr *instr, uint32 offset, uint32 targetOffset);
    BranchReloc *           CreateRelocRecord(IR::BranchInstr * branchInstr, uint32 offset, uint32 targetOffset);
    void                    BuildHeapBufferReload(uint32 offset);
    template<typename T, typename ConstOpnd, typename F> 
    void                    CreateLoadConstInstrForType(byte* table, Js::RegSlot& regAllocated, uint32 constCount, uint32 offset, IRType irType, ValueType valueType, Js::OpCode opcode, F extraProcess);
    void                    BuildConstantLoads();
    void                    BuildImplicitArgIns();
    void                    InsertLabels();
    IR::LabelInstr *        CreateLabel(IR::BranchInstr * branchInstr, uint& offset);
#if DBG
    BVFixed *               m_usedAsTemp;
#endif
    Js::RegSlot             GetTypedRegFromRegSlot(Js::RegSlot reg, WAsmJs::Types type);
    Js::RegSlot             GetRegSlotFromTypedReg(Js::RegSlot srcReg, WAsmJs::Types type);
    Js::RegSlot             GetRegSlotFromIntReg(Js::RegSlot srcIntReg) {return GetRegSlotFromTypedReg(srcIntReg, WAsmJs::INT32);}
    Js::RegSlot             GetRegSlotFromInt64Reg(Js::RegSlot srcIntReg) {return GetRegSlotFromTypedReg(srcIntReg, WAsmJs::INT64);}
    Js::RegSlot             GetRegSlotFromFloatReg(Js::RegSlot srcFloatReg) {return GetRegSlotFromTypedReg(srcFloatReg, WAsmJs::FLOAT32);}
    Js::RegSlot             GetRegSlotFromDoubleReg(Js::RegSlot srcDoubleReg) {return GetRegSlotFromTypedReg(srcDoubleReg, WAsmJs::FLOAT64);}
    Js::RegSlot             GetRegSlotFromSimd128Reg(Js::RegSlot srcSimd128Reg) {return GetRegSlotFromTypedReg(srcSimd128Reg, WAsmJs::SIMD);}

    Js::RegSlot             GetRegSlotFromVarReg(Js::RegSlot srcVarReg);
    Js::OpCode              GetSimdOpcode(Js::OpCodeAsmJs asmjsOpcode);
    void                    GetSimdTypesFromAsmType(Js::AsmJsType::Which asmType, IRType *pIRType, ValueType *pValueType = nullptr);
    IR::Instr *             AddExtendedArg(IR::RegOpnd *src1, IR::RegOpnd *src2, uint32 offset);
    bool                    RegIsSimd128ReturnVar(Js::RegSlot reg);
    SymID                   GetMappedTemp(Js::RegSlot reg);
    void                    SetMappedTemp(Js::RegSlot reg, SymID tempId);
    BOOL                    GetTempUsed(Js::RegSlot reg);
    void                    SetTempUsed(Js::RegSlot reg, BOOL used);
    BOOL                    RegIsTemp(Js::RegSlot reg);
    BOOL                    RegIsConstant(Js::RegSlot reg);
    BOOL                    RegIsVar(Js::RegSlot reg);
    BOOL                    RegIsTypedVar(Js::RegSlot reg, WAsmJs::Types type);
    BOOL                    RegIsIntVar(Js::RegSlot reg) {return RegIsTypedVar(reg, WAsmJs::INT32);}
    BOOL                    RegIsInt64Var(Js::RegSlot reg) {return RegIsTypedVar(reg, WAsmJs::INT64);}
    BOOL                    RegIsFloatVar(Js::RegSlot reg) {return RegIsTypedVar(reg, WAsmJs::FLOAT32);}
    BOOL                    RegIsDoubleVar(Js::RegSlot reg) {return RegIsTypedVar(reg, WAsmJs::FLOAT64);}
    BOOL                    RegIsSimd128Var(Js::RegSlot reg) {return RegIsTypedVar(reg, WAsmJs::SIMD);}

#define LAYOUT_TYPE(layout) \
    void                    Build##layout(Js::OpCodeAsmJs newOpcode, uint32 offset);
#define LAYOUT_TYPE_WMS(layout) \
    template <typename SizePolicy> void Build##layout(Js::OpCodeAsmJs newOpcode, uint32 offset);
#define EXCLUDE_FRONTEND_LAYOUT
#include "ByteCode/LayoutTypesAsmJs.h"
    void                    BuildSimd_1Ints(Js::OpCodeAsmJs newOpcode, uint32 offset, IRType dstSimdType, Js::RegSlot* srcRegSlots, Js::RegSlot dstRegSlot, uint LANES);
    void                    BuildSimd_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot, IRType simdType);
    void                    BuildSimd_2Int2(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot, Js::RegSlot src2RegSlot, Js::RegSlot src3RegSlot, IRType simdType);
    void                    BuildSimd_2(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot, IRType simdType);
    void                    BuildSimd_2Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot, Js::RegSlot src2RegSlot, IRType simdType);
    void                    BuildSimd_3(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot, Js::RegSlot src2RegSlot, IRType simdType);
    void                    BuildSimd_3(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot, Js::RegSlot src2RegSlot, IRType dstSimdType, IRType srcSimdType);
    void                    BuildSimdConversion(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot srcRegSlot, IRType dstSimdType, IRType srcSimdType);
    ValueType               GetSimdValueTypeFromIRType(IRType type);

    void                    BuildElementSlot(Js::OpCodeAsmJs newOpcode, uint32 offset, int32 slotIndex, Js::RegSlot value, Js::RegSlot instance);
    void                    BuildAsmUnsigned1(Js::OpCodeAsmJs newOpcode, uint value);
    void                    BuildAsmTypedArr(Js::OpCodeAsmJs newOpcode, uint32 offset, uint32 slotIndex, Js::RegSlot value, int8 viewType);
    void                    BuildAsmSimdTypedArr(Js::OpCodeAsmJs newOpcode, uint32 offset, uint32 slotIndex, Js::RegSlot value, int8 viewType, uint8 DataWidth);
    void                    BuildAsmCall(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::ArgSlot argCount, Js::RegSlot ret, Js::RegSlot function, int8 returnType);
    void                    BuildAsmReg1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstReg);
    void                    BuildBrInt1(Js::OpCodeAsmJs newOpcode, uint32 offset, int32 relativeOffset, Js::RegSlot src);
    void                    BuildBrInt2(Js::OpCodeAsmJs newOpcode, uint32 offset, int32 relativeOffset, Js::RegSlot src1, Js::RegSlot src2);
    void                    BuildBrInt1Const1(Js::OpCodeAsmJs newOpcode, uint32 offset, int32 relativeOffset, Js::RegSlot src1, int32 src2);
    void                    BuildBrCmp(Js::OpCodeAsmJs newOpcode, uint32 offset, int32 relativeOffset, IR::RegOpnd* src1Opnd, IR::Opnd* src2Opnd);
    void                    GenerateLoopBodySlotAccesses(uint offset);
    void                    GenerateLoopBodyStSlots(SymID loopParamSymId, uint offset);

    Js::PropertyId          CalculatePropertyOffset(SymID id, IRType type, bool isVar = true);

    IR::Instr*              GenerateStSlotForReturn(IR::RegOpnd* srcOpnd, IRType type);
    JitArenaAllocator *     m_tempAlloc;
    JitArenaAllocator *     m_funcAlloc;
    Func *                  m_func;
    IR::Instr *             m_lastInstr;
    IR::Instr **            m_offsetToInstruction;
    Js::ByteCodeReader      m_jnReader;
    Js::StatementReader     m_statementReader;
    SList<IR::Instr *> *    m_argStack;
    SList<IR::Instr *> *    m_tempList;
    SList<int32> *          m_argOffsetStack;
    SList<BranchReloc *> *  m_branchRelocList;
    // 1 for const, 1 for var, 1 for temps for each type and 1 for last
    static constexpr uint32 m_firstsTypeCount = WAsmJs::LIMIT * 3 + 1;
    Js::RegSlot             m_firstsType[m_firstsTypeCount];
    Js::RegSlot             m_firstVarConst;
    Js::RegSlot             m_firstIRTemp;
    Js::OpCode *            m_simdOpcodesMap;

    Js::RegSlot GetFirstConst(WAsmJs::Types type) { return m_firstsType[type]; }
    Js::RegSlot GetFirstVar(WAsmJs::Types type) { return m_firstsType[type + WAsmJs::LIMIT]; }
    Js::RegSlot GetFirstTmp(WAsmJs::Types type) { return m_firstsType[type + WAsmJs::LIMIT * 2]; }
    
    Js::RegSlot GetLastConst(WAsmJs::Types type) { return m_firstsType[type + 1]; }
    Js::RegSlot GetLastVar(WAsmJs::Types type) { return m_firstsType[type + WAsmJs::LIMIT + 1]; }
    Js::RegSlot GetLastTmp(WAsmJs::Types type) { return m_firstsType[type + WAsmJs::LIMIT * 2 + 1]; }

    SymID *                 m_tempMap;
    BVFixed *               m_fbvTempUsed;
    uint32                  m_functionStartOffset;
    Js::AsmJsFunctionInfo * m_asmFuncInfo;
    StackSym *              m_loopBodyRetIPSym;
    BVFixed *               m_ldSlots;
    BVFixed *               m_stSlots;
    BOOL                    m_IsTJLoopBody;
    IRBuilderAsmJsSwitchAdapter m_switchAdapter;
    SwitchIRBuilder         m_switchBuilder;
    IR::RegOpnd *           m_funcOpnd;
#if DBG
    uint32                  m_offsetToInstructionCount;
#endif

#define BUILD_LAYOUT_DEF(layout, ...) void Build##layout (Js::OpCodeAsmJs, uint32, __VA_ARGS__);
#define RegType Js::RegSlot
#define IntType Js::RegSlot
#define LongType Js::RegSlot
#define FloatType Js::RegSlot
#define DoubleType Js::RegSlot
#define IntConstType int
#define LongConstType int64
#define FloatConstType float
#define DoubleConstType double
#define Float32x4Type Js::RegSlot
#define Bool32x4Type Js::RegSlot
#define Int32x4Type Js::RegSlot
#define Float64x2Type Js::RegSlot
#define Int16x8Type Js::RegSlot
#define Bool16x8Type Js::RegSlot
#define Int8x16Type Js::RegSlot
#define Bool8x16Type Js::RegSlot
#define Uint32x4Type Js::RegSlot
#define Uint16x8Type Js::RegSlot
#define Uint8x16Type Js::RegSlot
#define LAYOUT_TYPE_WMS_REG2(layout, t0, t1) BUILD_LAYOUT_DEF(layout, t0##Type, t1##Type)
#define LAYOUT_TYPE_WMS_REG3(layout, t0, t1, t2) BUILD_LAYOUT_DEF(layout, t0##Type, t1##Type, t2##Type)
#define LAYOUT_TYPE_WMS_REG4(layout, t0, t1, t2, t3) BUILD_LAYOUT_DEF(layout, t0##Type, t1##Type, t2##Type, t3##Type)
#define LAYOUT_TYPE_WMS_REG5(layout, t0, t1, t2, t3, t4) BUILD_LAYOUT_DEF(layout, t0##Type, t1##Type, t2##Type, t3##Type, t4##Type)
#define LAYOUT_TYPE_WMS_REG6(layout, t0, t1, t2, t3, t4, t5) BUILD_LAYOUT_DEF(layout, t0##Type, t1##Type, t2##Type, t3##Type, t4##Type, t5##Type)
#define LAYOUT_TYPE_WMS_REG7(layout, t0, t1, t2, t3, t4, t5, t6) BUILD_LAYOUT_DEF(layout, t0##Type, t1##Type, t2##Type, t3##Type, t4##Type, t5##Type, t6##Type)
#define LAYOUT_TYPE_WMS_REG9(layout, t0, t1, t2, t3, t4, t5, t6, t7, t8) BUILD_LAYOUT_DEF(layout, t0##Type, t1##Type, t2##Type, t3##Type, t4##Type, t5##Type, t6##Type, t7##Type, t8##Type)
#define LAYOUT_TYPE_WMS_REG10(layout, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9) BUILD_LAYOUT_DEF(layout, t0##Type, t1##Type, t2##Type, t3##Type, t4##Type, t5##Type, t6##Type, t7##Type, t8##Type, t9##Type)
#define LAYOUT_TYPE_WMS_REG11(layout, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10) BUILD_LAYOUT_DEF(layout, t0##Type, t1##Type, t2##Type, t3##Type, t4##Type, t5##Type, t6##Type, t7##Type, t8##Type, t9##Type, t10##Type)
#define LAYOUT_TYPE_WMS_REG17(layout, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16) BUILD_LAYOUT_DEF(layout, t0##Type, t1##Type, t2##Type, t3##Type, t4##Type, t5##Type, t6##Type, t7##Type, t8##Type, t9##Type, t10##Type, t11##Type, t12##Type, t13##Type, t14##Type, t15##Type, t16##Type)
#define LAYOUT_TYPE_WMS_REG18(layout, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16, t17) BUILD_LAYOUT_DEF(layout, t0##Type, t1##Type, t2##Type, t3##Type, t4##Type, t5##Type, t6##Type, t7##Type, t8##Type, t9##Type, t10##Type, t11##Type, t12##Type, t13##Type, t14##Type, t15##Type, t16##Type, t17##Type)
#define LAYOUT_TYPE_WMS_REG19(layout, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16, t17, t18) BUILD_LAYOUT_DEF(layout, t0##Type, t1##Type, t2##Type, t3##Type, t4##Type, t5##Type, t6##Type, t7##Type, t8##Type, t9##Type, t10##Type, t11##Type, t12##Type, t13##Type, t14##Type, t15##Type, t16##Type, t17##Type, t18##Type)
#define EXCLUDE_FRONTEND_LAYOUT
#include "LayoutTypesAsmJs.h"
#undef BUILD_LAYOUT_DEF
#undef RegType
#undef IntType
#undef LongType
#undef FloatType
#undef DoubleType
#undef IntConstType
#undef LongConstType
#undef FloatConstType
#undef DoubleConstType
#undef Float32x4Type
#undef Bool32x4Type
#undef Int32x4Type
#undef Float64x2Type
#undef Int16x8Type
#undef Bool16x8Type
#undef Int8x16Type
#undef Bool8x16Type
#undef Uint32x4Type
#undef Uint16x8Type
#undef Uint8x16Type
};

#endif