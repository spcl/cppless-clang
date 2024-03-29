//===-- VVPISelLowering.cpp - VE DAG Lowering Implementation --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the lowering and legalization of vector instructions to
// VVP_*layer SDNodes.
//
//===----------------------------------------------------------------------===//

#include "VECustomDAG.h"
#include "VEISelLowering.h"

using namespace llvm;

#define DEBUG_TYPE "ve-lower"

SDValue VETargetLowering::legalizeInternalVectorOp(SDValue Op,
                                                   SelectionDAG &DAG) const {
  VECustomDAG CDAG(DAG, Op);

  EVT IdiomVT = Op.getValueType();
  if (isPackedVectorType(IdiomVT) &&
      !supportsPackedMode(Op.getOpcode(), IdiomVT))
    return splitVectorOp(Op, CDAG);

  // TODO: Implement odd/even splitting.
  return legalizePackedAVL(Op, CDAG);
}

SDValue VETargetLowering::splitVectorOp(SDValue Op, VECustomDAG &CDAG) const {
  MVT ResVT = splitVectorType(Op.getValue(0).getSimpleValueType());

  auto AVLPos = getAVLPos(Op->getOpcode());
  auto MaskPos = getMaskPos(Op->getOpcode());

  SDValue PackedMask = getNodeMask(Op);
  auto AVLPair = getAnnotatedNodeAVL(Op);
  SDValue PackedAVL = AVLPair.first;
  assert(!AVLPair.second && "Expecting non pack-legalized oepration");

  // request the parts
  SDValue PartOps[2];

  SDValue UpperPartAVL; // we will use this for packing things back together
  for (PackElem Part : {PackElem::Hi, PackElem::Lo}) {
    // VP ops already have an explicit mask and AVL. When expanding from non-VP
    // attach those additional inputs here.
    auto SplitTM = CDAG.getTargetSplitMask(PackedMask, PackedAVL, Part);

    if (Part == PackElem::Hi)
      UpperPartAVL = SplitTM.AVL;

    // Attach non-predicating value operands
    SmallVector<SDValue, 4> OpVec;
    for (unsigned i = 0; i < Op.getNumOperands(); ++i) {
      if (AVLPos && ((int)i) == *AVLPos)
        continue;
      if (MaskPos && ((int)i) == *MaskPos)
        continue;

      // Value operand
      auto PackedOperand = Op.getOperand(i);
      auto UnpackedOpVT = splitVectorType(PackedOperand.getSimpleValueType());
      SDValue PartV =
          CDAG.getUnpack(UnpackedOpVT, PackedOperand, Part, SplitTM.AVL);
      OpVec.push_back(PartV);
    }

    // Add predicating args and generate part node.
    OpVec.push_back(SplitTM.Mask);
    OpVec.push_back(SplitTM.AVL);
    // Emit legal VVP nodes.
    PartOps[(int)Part] =
        CDAG.getNode(Op.getOpcode(), ResVT, OpVec, Op->getFlags());
  }

  // Re-package vectors.
  return CDAG.getPack(Op.getValueType(), PartOps[(int)PackElem::Lo],
                      PartOps[(int)PackElem::Hi], UpperPartAVL);
}

SDValue VETargetLowering::legalizePackedAVL(SDValue Op,
                                            VECustomDAG &CDAG) const {
  LLVM_DEBUG(dbgs() << "::legalizePackedAVL\n";);
  // Only required for VEC and VVP ops.
  if (!isVVPOrVEC(Op->getOpcode()))
    return Op;

  // Operation already has a legal AVL.
  auto AVL = getNodeAVL(Op);
  if (isLegalAVL(AVL))
    return Op;

  // Half and round up EVL for 32bit element types.
  SDValue LegalAVL = AVL;
  if (isPackedVectorType(Op.getValueType())) {
    assert(maySafelyIgnoreMask(Op) &&
           "TODO Shift predication from EVL into Mask");

    if (auto *ConstAVL = dyn_cast<ConstantSDNode>(AVL)) {
      LegalAVL = CDAG.getConstant((ConstAVL->getZExtValue() + 1) / 2, MVT::i32);
    } else {
      auto ConstOne = CDAG.getConstant(1, MVT::i32);
      auto PlusOne = CDAG.getNode(ISD::ADD, MVT::i32, {AVL, ConstOne});
      LegalAVL = CDAG.getNode(ISD::SRL, MVT::i32, {PlusOne, ConstOne});
    }
  }

  SDValue AnnotatedLegalAVL = CDAG.annotateLegalAVL(LegalAVL);

  // Copy the operand list.
  int NumOp = Op->getNumOperands();
  auto AVLPos = getAVLPos(Op->getOpcode());
  std::vector<SDValue> FixedOperands;
  for (int i = 0; i < NumOp; ++i) {
    if (AVLPos && (i == *AVLPos)) {
      FixedOperands.push_back(AnnotatedLegalAVL);
      continue;
    }
    FixedOperands.push_back(Op->getOperand(i));
  }

  // Clone the operation with fixed operands.
  auto Flags = Op->getFlags();
  SDValue NewN =
      CDAG.getNode(Op->getOpcode(), Op->getVTList(), FixedOperands, Flags);
  return NewN;
}
