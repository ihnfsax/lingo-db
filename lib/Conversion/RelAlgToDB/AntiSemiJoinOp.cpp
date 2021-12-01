#include "mlir/Conversion/RelAlgToDB/ProducerConsumerNode.h"
#include "mlir/Dialect/DB/IR/DBOps.h"
#include "mlir/Dialect/RelAlg/IR/RelAlgOps.h"
#include "mlir/Dialect/util/UtilOps.h"
#include <mlir/Conversion/RelAlgToDB/HashJoinUtils.h>
#include <mlir/IR/BlockAndValueMapping.h>
class NLAntiSemiJoinLowering : public mlir::relalg::ProducerConsumerNode {
   mlir::relalg::AntiSemiJoinOp joinOp;
   mlir::Value matchFoundFlag;

   public:
   NLAntiSemiJoinLowering(mlir::relalg::AntiSemiJoinOp innerJoinOp) : mlir::relalg::ProducerConsumerNode(innerJoinOp), joinOp(innerJoinOp) {
   }

   virtual void consume(mlir::relalg::ProducerConsumerNode* child, mlir::OpBuilder& builder, mlir::relalg::LoweringContext& context) override {
      auto scope = context.createScope();
      if (child == this->children[0].get()) {
         matchFoundFlag = builder.create<mlir::db::CreateFlag>(joinOp->getLoc(), mlir::db::FlagType::get(builder.getContext()));
         children[1]->setFlag(matchFoundFlag);
         children[1]->produce(context, builder);
         mlir::Value matchFound = builder.create<mlir::db::GetFlag>(joinOp->getLoc(), mlir::db::BoolType::get(builder.getContext()), matchFoundFlag);
         mlir::Value noMatchFound = builder.create<mlir::db::NotOp>(joinOp->getLoc(), mlir::db::BoolType::get(builder.getContext()), matchFound);
         auto builderValuesBefore = getRequiredBuilderValues(context);
         auto ifOp = builder.create<mlir::db::IfOp>(
            joinOp->getLoc(), getRequiredBuilderTypes(context), noMatchFound, [&](mlir::OpBuilder& builder1, mlir::Location) {
               consumer->consume(this, builder1, context);
               builder1.create<mlir::db::YieldOp>(joinOp->getLoc(), getRequiredBuilderValues(context)); },
               requiredBuilders.empty() ? mlir::relalg::noBuilder : [&](mlir::OpBuilder& builder2, mlir::Location) { builder2.create<mlir::db::YieldOp>(joinOp->getLoc(), builderValuesBefore); });
         setRequiredBuilderValues(context,ifOp.getResults());
      } else if (child == this->children[1].get()) {
         mlir::Value matched= mergeRelationalBlock(
            builder.getInsertionBlock(), joinOp, [](auto x) { return &x->getRegion(0).front(); }, context, scope)[0];
         builder.create<mlir::db::SetFlag>(joinOp->getLoc(), matchFoundFlag, matched);
      }
   }
   virtual void produce(mlir::relalg::LoweringContext& context, mlir::OpBuilder& builder) override {
      children[0]->produce(context, builder);
   }

   virtual ~NLAntiSemiJoinLowering() {}
};

class HashAntiSemiJoinLowering : public mlir::relalg::HJNode {
   mlir::Value matchFoundFlag;

   public:
   HashAntiSemiJoinLowering(mlir::relalg::AntiSemiJoinOp innerJoinOp) : mlir::relalg::HJNode(innerJoinOp, innerJoinOp.right(), innerJoinOp.left()) {
   }

   virtual void handleLookup(mlir::Value matched, mlir::Value /*marker*/, mlir::relalg::LoweringContext& context, mlir::OpBuilder& builder) override {
      builder.create<mlir::db::SetFlag>(joinOp->getLoc(), matchFoundFlag, matched);
   }
   mlir::Value getFlag() override {
      return matchFoundFlag;
   }
   void beforeLookup(mlir::relalg::LoweringContext& context, mlir::OpBuilder& builder) override {
      matchFoundFlag = builder.create<mlir::db::CreateFlag>(joinOp->getLoc(), mlir::db::FlagType::get(builder.getContext()));
   }
   void afterLookup(mlir::relalg::LoweringContext& context, mlir::OpBuilder& builder) override {
      mlir::Value matchFound = builder.create<mlir::db::GetFlag>(joinOp->getLoc(), mlir::db::BoolType::get(builder.getContext()), matchFoundFlag);
      mlir::Value noMatchFound = builder.create<mlir::db::NotOp>(joinOp->getLoc(), mlir::db::BoolType::get(builder.getContext()), matchFound);
      handlePotentialMatch(builder,context,noMatchFound);
   }
   virtual ~HashAntiSemiJoinLowering() {}
};
class MHashAntiSemiJoinLowering : public mlir::relalg::HJNode {
   public:
   MHashAntiSemiJoinLowering(mlir::relalg::AntiSemiJoinOp innerJoinOp) : mlir::relalg::HJNode(innerJoinOp, innerJoinOp.left(), innerJoinOp.right(),true) {
   }

   virtual void handleLookup(mlir::Value matched, mlir::Value markerPtr, mlir::relalg::LoweringContext& context, mlir::OpBuilder& builder) override {
      auto builderValuesBefore = getRequiredBuilderValues(context);
      auto ifOp = builder.create<mlir::db::IfOp>(joinOp->getLoc(), getRequiredBuilderTypes(context), matched,[&](mlir::OpBuilder& builder1,mlir::Location loc){
         auto const1 = builder1.create<mlir::arith::ConstantOp>(builder1.getUnknownLoc(), builder1.getIntegerType(64), builder1.getI64IntegerAttr(1));
         builder1.create<mlir::AtomicRMWOp>(builder1.getUnknownLoc(), builder1.getIntegerType(64), mlir::AtomicRMWKind::assign, const1, markerPtr, mlir::ValueRange{});
         builder1.create<mlir::db::YieldOp>(joinOp->getLoc(), getRequiredBuilderValues(context));
         },requiredBuilders.empty() ? mlir::relalg::noBuilder : [&](mlir::OpBuilder& builder2, mlir::Location) { builder2.create<mlir::db::YieldOp>(joinOp->getLoc(), builderValuesBefore); });
      setRequiredBuilderValues(context,ifOp.getResults());
   }
   virtual void after(mlir::relalg::LoweringContext& context, mlir::OpBuilder& builder) override {
      scanHT(context, builder);
   }
   void handleScanned(mlir::Value marker, mlir::relalg::LoweringContext& context, mlir::OpBuilder& builder) override {
      auto zero = builder.create<mlir::arith::ConstantOp>(builder.getUnknownLoc(), marker.getType(), builder.getIntegerAttr(marker.getType(), 0));
      auto isZero = builder.create<mlir::arith::CmpIOp>(builder.getUnknownLoc(), mlir::arith::CmpIPredicate::eq, marker, zero);
      auto isZeroDB = builder.create<mlir::db::TypeCastOp>(builder.getUnknownLoc(), mlir::db::BoolType::get(builder.getContext()), isZero);
      handlePotentialMatch(builder,context,isZeroDB);
   }

   virtual ~MHashAntiSemiJoinLowering() {}
};
bool mlir::relalg::ProducerConsumerNodeRegistry::registeredAntiSemiJoinOp = mlir::relalg::ProducerConsumerNodeRegistry::registerNode([](mlir::relalg::AntiSemiJoinOp joinOp) {
   if (joinOp->hasAttr("impl")) {
      if (auto impl = joinOp->getAttr("impl").dyn_cast_or_null<mlir::StringAttr>()) {
         if (impl.getValue() == "hash") {
            return (std::unique_ptr<mlir::relalg::ProducerConsumerNode>) std::make_unique<HashAntiSemiJoinLowering>(joinOp);
         }
         if (impl.getValue() == "markhash") {
            return (std::unique_ptr<mlir::relalg::ProducerConsumerNode>) std::make_unique<MHashAntiSemiJoinLowering>(joinOp);
         }
      }
   }
   return (std::unique_ptr<mlir::relalg::ProducerConsumerNode>) std::make_unique<NLAntiSemiJoinLowering>(joinOp);
});