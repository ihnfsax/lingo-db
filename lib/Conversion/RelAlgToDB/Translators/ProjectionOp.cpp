#include "mlir/Conversion/RelAlgToDB/Translator.h"
#include "mlir/Dialect/DB/IR/DBOps.h"
#include "mlir/Dialect/RelAlg/IR/RelAlgOps.h"
#include "mlir/Dialect/util/UtilOps.h"
#include "mlir/Conversion/RelAlgToDB/OrderedAttributes.h"

class ProjectionTranslator : public mlir::relalg::Translator {
   mlir::relalg::ProjectionOp projectionOp;

   public:
   ProjectionTranslator(mlir::relalg::ProjectionOp projectionOp) : mlir::relalg::Translator(projectionOp), projectionOp(projectionOp) {
   }
   virtual void addRequiredBuilders(std::vector<size_t> requiredBuilders) override {
      this->requiredBuilders.insert(this->requiredBuilders.end(), requiredBuilders.begin(), requiredBuilders.end());
      children[0]->addRequiredBuilders(requiredBuilders);
   }

   virtual void consume(mlir::relalg::Translator* child, mlir::OpBuilder& builder, mlir::relalg::TranslatorContext& context) override {
      auto scope = context.createScope();
      consumer->consume(this, builder, context);
   }
   virtual void produce(mlir::relalg::TranslatorContext& context, mlir::OpBuilder& builder) override {
      children[0]->produce(context, builder);
   }

   virtual ~ProjectionTranslator() {}
};

class DistinctProjectionTranslator : public mlir::relalg::Translator {
   mlir::relalg::ProjectionOp projectionOp;
   size_t builderId;
   mlir::Value table;
   mlir::relalg::OrderedAttributes key;

   mlir::TupleType valTupleType;
   mlir::TupleType entryType;


   public:
   DistinctProjectionTranslator(mlir::relalg::ProjectionOp projectionOp) : mlir::relalg::Translator(projectionOp), projectionOp(projectionOp) {
   }

   virtual void addRequiredBuilders(std::vector<size_t> requiredBuilders) override {
      this->requiredBuilders.insert(this->requiredBuilders.end(), requiredBuilders.begin(), requiredBuilders.end());
   }
   virtual void consume(mlir::relalg::Translator* child, mlir::OpBuilder& builder, mlir::relalg::TranslatorContext& context) override {
      mlir::Value htBuilder = context.builders[builderId];
      mlir::Value emptyVals = builder.create<mlir::util::UndefTupleOp>(projectionOp->getLoc(), valTupleType);
      mlir::Value packedKey = key.pack(context, builder, projectionOp->getLoc());
      mlir::Value packed = builder.create<mlir::util::PackOp>(projectionOp->getLoc(), mlir::ValueRange({packedKey, emptyVals}));

      auto mergedBuilder = builder.create<mlir::db::BuilderMerge>(projectionOp->getLoc(), htBuilder.getType(), htBuilder, packed);
      mlir::Block* aggrBuilderBlock = new mlir::Block;
      mergedBuilder.fn().push_back(aggrBuilderBlock);
      aggrBuilderBlock->addArguments({valTupleType, valTupleType}, {projectionOp->getLoc(),projectionOp->getLoc()});
      mlir::OpBuilder builder2(builder.getContext());
      builder2.setInsertionPointToStart(aggrBuilderBlock);
      builder2.create<mlir::db::YieldOp>(projectionOp->getLoc(), aggrBuilderBlock->getArgument(0));
      context.builders[builderId] = mergedBuilder.result_builder();
   }

   virtual void produce(mlir::relalg::TranslatorContext& context, mlir::OpBuilder& builder) override {
      auto scope = context.createScope();
      key=mlir::relalg::OrderedAttributes::fromRefArr(projectionOp.attrs());
      valTupleType = mlir::TupleType::get(builder.getContext(), {});
      auto keyTupleType=key.getTupleType(builder.getContext());
      auto parentPipeline = context.pipelineManager.getCurrentPipeline();
      auto p = std::make_shared<mlir::relalg::Pipeline>(builder.getBlock()->getParentOp()->getParentOfType<mlir::ModuleOp>());
      context.pipelineManager.setCurrentPipeline(p);
      context.pipelineManager.addPipeline(p);
      auto res = p->addInitFn([&](mlir::OpBuilder& builder) {
         mlir::Value emptyTuple = builder.create<mlir::util::UndefTupleOp>(projectionOp.getLoc(), mlir::TupleType::get(builder.getContext()));
         auto aggrBuilder = builder.create<mlir::db::CreateAggrHTBuilder>(projectionOp.getLoc(), mlir::db::AggrHTBuilderType::get(builder.getContext(), keyTupleType, valTupleType, valTupleType), emptyTuple);
         return std::vector<mlir::Value>({aggrBuilder});
      });
      builderId = context.getBuilderId();
      context.builders[builderId] = p->addDependency(res[0]);
      entryType = mlir::TupleType::get(builder.getContext(), {keyTupleType, valTupleType});
      children[0]->addRequiredBuilders({builderId});
      children[0]->produce(context, p->getBuilder());
      p->finishMainFunction({context.builders[builderId]});
      auto hashtableRes = p->addFinalizeFn([&](mlir::OpBuilder& builder, mlir::ValueRange args) {
         mlir::Value hashtable = builder.create<mlir::db::BuilderBuild>(projectionOp.getLoc(), mlir::db::AggregationHashtableType::get(builder.getContext(), keyTupleType, valTupleType), args[0]);
         return std::vector<mlir::Value>{hashtable};
      });
      context.pipelineManager.setCurrentPipeline(parentPipeline);

      {
         auto forOp2 = builder.create<mlir::db::ForOp>(projectionOp->getLoc(), getRequiredBuilderTypes(context), context.pipelineManager.getCurrentPipeline()->addDependency(hashtableRes[0]), context.pipelineManager.getCurrentPipeline()->getFlag(), getRequiredBuilderValues(context));
         mlir::Block* block2 = new mlir::Block;
         block2->addArgument(entryType, projectionOp->getLoc());
         block2->addArguments(getRequiredBuilderTypes(context), getRequiredBuilderLocs(context));
         forOp2.getBodyRegion().push_back(block2);
         mlir::OpBuilder builder2(forOp2.getBodyRegion());
         setRequiredBuilderValues(context, block2->getArguments().drop_front(1));
         auto unpacked = builder2.create<mlir::util::UnPackOp>(projectionOp->getLoc(), forOp2.getInductionVar()).getResults();
         auto unpackedKey = builder2.create<mlir::util::UnPackOp>(projectionOp->getLoc(), unpacked[0]).getResults();
         key.setValuesForAttributes(context,scope,unpackedKey);
         consumer->consume(this, builder2, context);
         builder2.create<mlir::db::YieldOp>(projectionOp->getLoc(), getRequiredBuilderValues(context));
         setRequiredBuilderValues(context, forOp2.results());
      }
      builder.create<mlir::db::FreeOp>(projectionOp->getLoc(), context.pipelineManager.getCurrentPipeline()->addDependency(hashtableRes[0]));
   }
   virtual void done() override {
   }
   virtual ~DistinctProjectionTranslator() {}
};
std::unique_ptr<mlir::relalg::Translator> mlir::relalg::Translator::createProjectionTranslator(mlir::relalg::ProjectionOp projectionOp) {
   if (projectionOp.set_semantic() == mlir::relalg::SetSemantic::distinct) {
      return (std::unique_ptr<Translator>) std::make_unique<DistinctProjectionTranslator>(projectionOp);
   } else {
      return (std::unique_ptr<Translator>) std::make_unique<ProjectionTranslator>(projectionOp);
   }
}