#include "mlir/Conversion/RelAlgToDB/ProducerConsumerNode.h"
#include "mlir/Dialect/DB/IR/DBOps.h"
#include "mlir/Dialect/RelAlg/IR/RelAlgOps.h"
#include "mlir/Dialect/util/UtilOps.h"

class ProjectionLowering : public mlir::relalg::ProducerConsumerNode {
   mlir::relalg::ProjectionOp projectionOp;

   public:
   ProjectionLowering(mlir::relalg::ProjectionOp projectionOp) : mlir::relalg::ProducerConsumerNode(projectionOp), projectionOp(projectionOp) {
   }
   virtual void addRequiredBuilders(std::vector<size_t> requiredBuilders) override{
      this->requiredBuilders.insert(this->requiredBuilders.end(), requiredBuilders.begin(), requiredBuilders.end());
      children[0]->addRequiredBuilders(requiredBuilders);
   }

   virtual void consume(mlir::relalg::ProducerConsumerNode* child, mlir::OpBuilder& builder, mlir::relalg::LoweringContext& context) override {
      auto scope = context.createScope();
      consumer->consume(this, builder, context);
   }
   virtual void produce(mlir::relalg::LoweringContext& context, mlir::OpBuilder& builder) override {
      children[0]->produce(context, builder);
   }

   virtual ~ProjectionLowering() {}
};

class DistinctProjectionLowering : public mlir::relalg::ProducerConsumerNode {
   mlir::relalg::ProjectionOp projectionOp;
   size_t builderId;
   mlir::Value table;
   std::vector<mlir::Type> keyTypes;
   mlir::TupleType keyTupleType;
   mlir::TupleType valTupleType;
   mlir::TupleType entryType;

   std::vector<const mlir::relalg::RelationalAttribute*> groupAttributes;
   std::unordered_map<const mlir::relalg::RelationalAttribute*, size_t> keyMapping;

   public:
   DistinctProjectionLowering(mlir::relalg::ProjectionOp projectionOp) : mlir::relalg::ProducerConsumerNode(projectionOp), projectionOp(projectionOp) {
   }

   virtual void addRequiredBuilders(std::vector<size_t> requiredBuilders) override{
      this->requiredBuilders.insert(this->requiredBuilders.end(), requiredBuilders.begin(), requiredBuilders.end());
   }
   virtual void consume(mlir::relalg::ProducerConsumerNode* child, mlir::OpBuilder& builder, mlir::relalg::LoweringContext& context) override {
      mlir::Value htBuilder = context.builders[builderId];
      mlir::Value emptyVals = builder.create<mlir::util::UndefTupleOp>(projectionOp->getLoc(), valTupleType);
      mlir::Value packedKey = packValues(context,builder,groupAttributes);
      mlir::Value packed = builder.create<mlir::util::PackOp>(projectionOp->getLoc(), mlir::ValueRange({packedKey, emptyVals}));

      auto mergedBuilder = builder.create<mlir::db::BuilderMerge>(projectionOp->getLoc(), htBuilder.getType(), htBuilder, packed);
      mlir::Block* aggrBuilderBlock = new mlir::Block;
      mergedBuilder.fn().push_back(aggrBuilderBlock);
      aggrBuilderBlock->addArguments({valTupleType, valTupleType});
      mlir::OpBuilder builder2(builder.getContext());
      builder2.setInsertionPointToStart(aggrBuilderBlock);
      builder2.create<mlir::db::YieldOp>(builder.getUnknownLoc(), aggrBuilderBlock->getArgument(0));
      context.builders[builderId] = mergedBuilder.result_builder();
   }

   virtual void produce(mlir::relalg::LoweringContext& context, mlir::OpBuilder& builder) override {
      auto scope = context.createScope();

      for (auto attr : projectionOp.attrs()) {
         if (auto attrRef = attr.dyn_cast_or_null<mlir::relalg::RelationalAttributeRefAttr>()) {
            keyMapping[&attrRef.getRelationalAttribute()] = keyTypes.size();
            keyTypes.push_back(attrRef.getRelationalAttribute().type);
            groupAttributes.push_back(&attrRef.getRelationalAttribute());
         }
      }
      keyTupleType = mlir::TupleType::get(builder.getContext(), keyTypes);
      valTupleType = mlir::TupleType::get(builder.getContext(), {});
      mlir::Value emptyTuple=builder.create<mlir::util::UndefTupleOp>(projectionOp.getLoc(),mlir::TupleType::get(builder.getContext()));
      auto aggrBuilder = builder.create<mlir::db::CreateAggrHTBuilder>(projectionOp.getLoc(), mlir::db::AggrHTBuilderType::get(builder.getContext(), keyTupleType, valTupleType,valTupleType),emptyTuple);
      builderId = context.getBuilderId();
      context.builders[builderId] = aggrBuilder;
      entryType = mlir::TupleType::get(builder.getContext(), {keyTupleType, valTupleType});
      children[0]->addRequiredBuilders({builderId});
      children[0]->produce(context, builder);
      mlir::Value hashtable = builder.create<mlir::db::BuilderBuild>(projectionOp.getLoc(), mlir::db::AggregationHashtableType::get(builder.getContext(), keyTupleType, valTupleType), context.builders[builderId]);
      {
         auto forOp2 = builder.create<mlir::db::ForOp>(projectionOp->getLoc(), getRequiredBuilderTypes(context), hashtable, flag, getRequiredBuilderValues(context));
         mlir::Block* block2 = new mlir::Block;
         block2->addArgument(entryType);
         block2->addArguments(getRequiredBuilderTypes(context));
         forOp2.getBodyRegion().push_back(block2);
         mlir::OpBuilder builder2(forOp2.getBodyRegion());
         setRequiredBuilderValues(context, block2->getArguments().drop_front(1));
         auto unpacked = builder2.create<mlir::util::UnPackOp>(projectionOp->getLoc(), forOp2.getInductionVar()).getResults();
         auto unpackedKey = builder2.create<mlir::util::UnPackOp>(projectionOp->getLoc(), unpacked[0]).getResults();

         for (const auto* attr : requiredAttributes) {
            if (keyMapping.count(attr)) {
               context.setValueForAttribute(scope, attr, unpackedKey[keyMapping[attr]]);
            }
         }
         consumer->consume(this, builder2, context);
         builder2.create<mlir::db::YieldOp>(projectionOp->getLoc(), getRequiredBuilderValues(context));
         setRequiredBuilderValues(context, forOp2.results());
      }
      builder.create<mlir::db::FreeOp>(projectionOp->getLoc(),hashtable);
   }
   virtual void done() override {
   }
   virtual ~DistinctProjectionLowering() {}
};
bool mlir::relalg::ProducerConsumerNodeRegistry::registeredProjectionOp = mlir::relalg::ProducerConsumerNodeRegistry::registerNode([](mlir::relalg::ProjectionOp projectionOp) {
   if (projectionOp.set_semantic() == mlir::relalg::SetSemantic::distinct) {
      return (std::unique_ptr<ProducerConsumerNode>) std::make_unique<DistinctProjectionLowering>(projectionOp);
   } else {
      return (std::unique_ptr<ProducerConsumerNode>) std::make_unique<ProjectionLowering>(projectionOp);
   }
});