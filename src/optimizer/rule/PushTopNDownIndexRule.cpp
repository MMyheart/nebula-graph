/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#include "optimizer/rule/PushTopNDownIndexRule.h"

#include "common/expression/FunctionCallExpression.h"
#include "common/interface/gen-cpp2/storage_types.h"
#include "optimizer/OptRule.h"
#include "optimizer/OptContext.h"
#include "optimizer/OptGroup.h"
#include "planner/plan/PlanNode.h"
#include "planner/plan/Query.h"

using nebula::graph::Project;
using nebula::graph::PlanNode;
using nebula::graph::TopN;
using nebula::graph::QueryContext;
using nebula::graph::IndexScan;
using nebula::graph::TagIndexFullScan;
using nebula::storage::cpp2::IndexQueryContext;

namespace nebula {
namespace opt {

int64_t PushTopNDownIndexRule::limit(const MatchedResult& matched) const {
    auto topN = static_cast<const TopN *>(matched.node->node());
    int limitRows = topN->offset() + topN->count();
    auto indexScan = static_cast<const IndexScan*>(matched.planNode({0, 0, 0}));
    if (indexScan->limit() >= 0 && limitRows >= indexScan->limit()) {
        limitRows = indexScan->limit();
    }
    return limitRows;
}
StatusOr<std::vector<storage::cpp2::OrderBy>> PushTopNDownIndexRule::orderBy(
    const MatchedResult& matched) const {
    auto indexScan = static_cast<const IndexScan *>(matched.planNode({0, 0, 0}));
    if (!indexScan->orderBy().empty()) {
        return Status::NotSupported();
    }
    auto indexReturnColNames = indexScan->returnColumns();
    auto proj = static_cast<const Project *>(matched.planNode({0, 0}));
    auto projColNames = proj->colNames();

    auto topN = static_cast<const TopN *>(matched.node->node());
    auto &factors = topN->factors();
    std::vector<storage::cpp2::OrderBy> orderBys;
    orderBys.reserve(factors.size());
    for (auto factor : factors) {
        auto colName = projColNames[factor.first];
        auto eq = [&](const std::string& name) { return name == colName; };
        auto iter = std::find_if(indexReturnColNames.cbegin(), indexReturnColNames.cend(), eq);
        size_t colIdx = std::distance(indexReturnColNames.cbegin(), iter);
        storage::cpp2::OrderBy orderBy;
        orderBy.set_pos(colIdx);
        orderBy.set_prop("");
        orderBy.set_direction(factor.second == OrderFactor::OrderType::ASCEND
                                  ? storage::cpp2::OrderDirection::ASCENDING
                                  : storage::cpp2::OrderDirection::DESCENDING);
        orderBys.emplace_back(orderBy);
    }
    return orderBys;
}
OptGroupNode*PushTopNDownIndexRule::topN(OptContext *ctx,
                                                     const MatchedResult& matched) const {
    auto topNGroupNode = matched.node;
    const auto topN = static_cast<const TopN *>(topNGroupNode->node());
    auto newTopN = static_cast<TopN *>(topN->clone());
    return OptGroupNode::create(ctx, newTopN, topNGroupNode->group());
}


std::unique_ptr<OptRule> PushTopNDownTagIndexFullScanRule::kInstance =
    std::unique_ptr<PushTopNDownTagIndexFullScanRule>(new PushTopNDownTagIndexFullScanRule());
PushTopNDownTagIndexFullScanRule::PushTopNDownTagIndexFullScanRule() {
    RuleSet::DefaultRules().addRule(this);
}
const Pattern &PushTopNDownTagIndexFullScanRule::pattern() const {
    static Pattern pattern =
        Pattern::create(graph::PlanNode::Kind::kTopN,
        {Pattern::create(graph::PlanNode::Kind::kProject,
                 {Pattern::create(graph::PlanNode::Kind::kTagIndexFullScan)})});
    return pattern;
}
std::string PushTopNDownTagIndexFullScanRule::toString() const {
    return "PushTopNDownTagIndexFullScanRule";
}


std::unique_ptr<OptRule> PushTopNDownTagIndexPrefixScanRule::kInstance =
    std::unique_ptr<PushTopNDownTagIndexPrefixScanRule>(new PushTopNDownTagIndexPrefixScanRule());
PushTopNDownTagIndexPrefixScanRule::PushTopNDownTagIndexPrefixScanRule() {
    RuleSet::DefaultRules().addRule(this);
}
const Pattern &PushTopNDownTagIndexPrefixScanRule::pattern() const {
    static Pattern pattern =
        Pattern::create(graph::PlanNode::Kind::kTopN,
        {Pattern::create(graph::PlanNode::Kind::kProject,
                 {Pattern::create(graph::PlanNode::Kind::kTagIndexPrefixScan)})});
    return pattern;
}
std::string PushTopNDownTagIndexPrefixScanRule::toString() const {
    return "PushTopNDownTagIndexPrefixScanRule";
}


std::unique_ptr<OptRule> PushTopNDownTagIndexRangeScanRule::kInstance =
    std::unique_ptr<PushTopNDownTagIndexRangeScanRule>(new PushTopNDownTagIndexRangeScanRule());
PushTopNDownTagIndexRangeScanRule::PushTopNDownTagIndexRangeScanRule() {
    RuleSet::DefaultRules().addRule(this);
}
const Pattern &PushTopNDownTagIndexRangeScanRule::pattern() const {
    static Pattern pattern =
        Pattern::create(graph::PlanNode::Kind::kTopN,
        {Pattern::create(graph::PlanNode::Kind::kProject,
                 {Pattern::create(graph::PlanNode::Kind::kTagIndexRangeScan)})});
    return pattern;
}
std::string PushTopNDownTagIndexRangeScanRule::toString() const {
    return "PushTopNDownTagIndexRangeScanRule";
}


std::unique_ptr<OptRule> PushTopNDownEdgeIndexFullScanRule::kInstance =
    std::unique_ptr<PushTopNDownEdgeIndexFullScanRule>(new PushTopNDownEdgeIndexFullScanRule());
PushTopNDownEdgeIndexFullScanRule::PushTopNDownEdgeIndexFullScanRule() {
    RuleSet::DefaultRules().addRule(this);
}
const Pattern &PushTopNDownEdgeIndexFullScanRule::pattern() const {
    static Pattern pattern =
        Pattern::create(graph::PlanNode::Kind::kTopN,
        {Pattern::create(graph::PlanNode::Kind::kProject,
                 {Pattern::create(graph::PlanNode::Kind::kEdgeIndexFullScan)})});
    return pattern;
}
std::string PushTopNDownEdgeIndexFullScanRule::toString() const {
    return "PushTopNDownEdgeIndexFullScanRule";
}


std::unique_ptr<OptRule> PushTopNDownEdgeIndexPrefixScanRule::kInstance =
    std::unique_ptr<PushTopNDownEdgeIndexPrefixScanRule>(new PushTopNDownEdgeIndexPrefixScanRule());
PushTopNDownEdgeIndexPrefixScanRule::PushTopNDownEdgeIndexPrefixScanRule() {
    RuleSet::DefaultRules().addRule(this);
}
const Pattern &PushTopNDownEdgeIndexPrefixScanRule::pattern() const {
    static Pattern pattern =
        Pattern::create(graph::PlanNode::Kind::kTopN,
        {Pattern::create(graph::PlanNode::Kind::kProject,
                 {Pattern::create(graph::PlanNode::Kind::kEdgeIndexPrefixScan)})});
    return pattern;
}
std::string PushTopNDownEdgeIndexPrefixScanRule::toString() const {
    return "PushTopNDownEdgeIndexPrefixScanRule";
}


std::unique_ptr<OptRule> PushTopNDownEdgeIndexRangeScanRule::kInstance =
    std::unique_ptr<PushTopNDownEdgeIndexRangeScanRule>(new PushTopNDownEdgeIndexRangeScanRule());
PushTopNDownEdgeIndexRangeScanRule::PushTopNDownEdgeIndexRangeScanRule() {
    RuleSet::DefaultRules().addRule(this);
}
const Pattern &PushTopNDownEdgeIndexRangeScanRule::pattern() const {
    static Pattern pattern =
        Pattern::create(graph::PlanNode::Kind::kTopN,
        {Pattern::create(graph::PlanNode::Kind::kProject,
                 {Pattern::create(graph::PlanNode::Kind::kEdgeIndexRangeScan)})});
    return pattern;
}
std::string PushTopNDownEdgeIndexRangeScanRule::toString() const {
    return "PushTopNDownEdgeIndexRangeScanRule";
}


std::unique_ptr<OptRule> PushTopNDownIndexScanRule::kInstance =
    std::unique_ptr<PushTopNDownIndexScanRule>(new PushTopNDownIndexScanRule());
PushTopNDownIndexScanRule::PushTopNDownIndexScanRule() {
    RuleSet::DefaultRules().addRule(this);
}
const Pattern &PushTopNDownIndexScanRule::pattern() const {
    static Pattern pattern =
        Pattern::create(graph::PlanNode::Kind::kTopN,
        {Pattern::create(graph::PlanNode::Kind::kProject,
                 {Pattern::create(graph::PlanNode::Kind::kIndexScan)})});
    return pattern;
}
std::string PushTopNDownIndexScanRule::toString() const {
    return "PushTopNDownIndexScanRule";
}

}   // namespace opt
}   // namespace nebula
