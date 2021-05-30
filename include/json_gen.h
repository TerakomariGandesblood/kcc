#pragma once

#include <string>
#include <string_view>

#include <boost/json.hpp>

#include "ast.h"
#include "visitor.h"

namespace kcc {

class JsonGen : public Visitor {
 public:
  explicit JsonGen(const std::string &filter = "");
  void GenJson(const TranslationUnit *root, const std::string &file_name);

 private:
  bool CheckFileName(const AstNode *node) const;

  virtual void Visit(const UnaryOpExpr *node) override;
  virtual void Visit(const TypeCastExpr *node) override;
  virtual void Visit(const BinaryOpExpr *node) override;
  virtual void Visit(const ConditionOpExpr *node) override;
  virtual void Visit(const FuncCallExpr *node) override;
  virtual void Visit(const ConstantExpr *node) override;
  virtual void Visit(const StringLiteralExpr *node) override;
  virtual void Visit(const IdentifierExpr *node) override;
  virtual void Visit(const EnumeratorExpr *node) override;
  virtual void Visit(const ObjectExpr *node) override;
  virtual void Visit(const StmtExpr *node) override;

  virtual void Visit(const LabelStmt *node) override;
  virtual void Visit(const CaseStmt *node) override;
  virtual void Visit(const DefaultStmt *node) override;
  virtual void Visit(const CompoundStmt *node) override;
  virtual void Visit(const ExprStmt *node) override;
  virtual void Visit(const IfStmt *node) override;
  virtual void Visit(const SwitchStmt *node) override;
  virtual void Visit(const WhileStmt *node) override;
  virtual void Visit(const DoWhileStmt *node) override;
  virtual void Visit(const ForStmt *node) override;
  virtual void Visit(const GotoStmt *node) override;
  virtual void Visit(const ContinueStmt *node) override;
  virtual void Visit(const BreakStmt *node) override;
  virtual void Visit(const ReturnStmt *node) override;

  virtual void Visit(const TranslationUnit *node) override;
  virtual void Visit(const Declaration *node) override;
  virtual void Visit(const FuncDef *node) override;

  std::string filter_;
  boost::json::value result_;

  constexpr static std::string_view Before{
      "<!DOCTYPE html>\n"
      "<html lang=\"zh\">\n"
      "\n"
      "<head>\n"
      "  <meta charset=\"UTF-8\">\n"
      "  <title>Abstract syntax tree</title>\n"
      "</head>\n"
      "\n"
      "<style>\n"
      "  .node circle {\n"
      "    fill: #fff;\n"
      "    stroke: steelblue;\n"
      "    stroke-width: 4px;\n"
      "  }\n"
      "\n"
      "  .node text {\n"
      "    font: 16px sans-serif;\n"
      "  }\n"
      "\n"
      "  .link {\n"
      "    fill: none;\n"
      "    stroke: #ccc;\n"
      "    stroke-width: 3px;\n"
      "  }\n"
      "</style>\n"
      "\n"
      "<body>\n"
      "  <!-- load the d3.js library -->\n"
      "  <script src=\"https://d3js.org/d3.v5.min.js\"></script>\n"
      "  <script>\n"
      "\n"
      "    var treeData = "};

  constexpr static std::string_view After{
      "// Set the dimensions and margins of the diagram\n"
      "    var margin = { top: 20, right: 90, bottom: 30, left: 120 },\n"
      "      width = 5000 - margin.left - margin.right,\n"
      "      height = 850 - margin.top - margin.bottom;\n"
      "\n"
      "    // append the svg object to the body of the page\n"
      "    // appends a 'group' element to 'svg'\n"
      "    // moves the 'group' element to the top left margin\n"
      "    var svg = d3.select(\"body\").append(\"svg\")\n"
      "      .attr(\"width\", width + margin.right + margin.left)\n"
      "      .attr(\"height\", height + margin.top + margin.bottom)\n"
      "      .append(\"g\")\n"
      "      .attr(\"transform\", \"translate(\"\n"
      "        + margin.left + \",\" + margin.top + \")\");\n"
      "\n"
      "    var i = 0,\n"
      "      duration = 750,\n"
      "      root;\n"
      "\n"
      "    // declares a tree layout and assigns the size\n"
      "    var treemap = d3.tree().size([height, width]);\n"
      "\n"
      "    // Assigns parent, children, height, depth\n"
      "    root = d3.hierarchy(treeData, function (d) { return d.children; "
      "});\n"
      "    root.x0 = height / 2;\n"
      "    root.y0 = 0;\n"
      "\n"
      "    // Collapse after the second level\n"
      "    //root.children.forEach(collapse);\n"
      "\n"
      "    update(root);\n"
      "\n"
      "    // Collapse the node and all it's children\n"
      "    // function collapse(d) {\n"
      "    //   if(d.children) {\n"
      "    //     d._children = d.children\n"
      "    //     d._children.forEach(collapse)\n"
      "    //     d.children = null\n"
      "    //   }\n"
      "    // }\n"
      "\n"
      "    function update(source) {\n"
      "\n"
      "      // Assigns the x and y position for the nodes\n"
      "      var treeData = treemap(root);\n"
      "\n"
      "      // Compute the new tree layout.\n"
      "      var nodes = treeData.descendants(),\n"
      "        links = treeData.descendants().slice(1);\n"
      "\n"
      "      // Normalize for fixed-depth.\n"
      "      nodes.forEach(function (d) { d.y = d.depth * 180 });\n"
      "\n"
      "      // ****************** Nodes section ***************************\n"
      "\n"
      "      // Update the nodes...\n"
      "      var node = svg.selectAll('g.node')\n"
      "        .data(nodes, function (d) { return d.id || (d.id = ++i); });\n"
      "\n"
      "      // Enter any new modes at the parent's previous position.\n"
      "      var nodeEnter = node.enter().append('g')\n"
      "        .attr('class', 'node')\n"
      "        .attr(\"transform\", function (d) {\n"
      "          return \"translate(\" + source.y0 + \",\" + source.x0 + "
      "\")\";\n"
      "        })\n"
      "        .on('click', click);\n"
      "\n"
      "      // Add Circle for the nodes\n"
      "      nodeEnter.append('circle')\n"
      "        .attr('class', 'node')\n"
      "        .attr('r', 1e-6)\n"
      "        .style(\"fill\", function (d) {\n"
      "          return d._children ? \"lightsteelblue\" : \"#fff\";\n"
      "        });\n"
      "\n"
      "      // Add labels for the nodes\n"
      "      nodeEnter.append('text')\n"
      "        .attr(\"dy\", \".35em\")\n"
      "        .attr(\"x\", function (d) {\n"
      "          return d.children || d._children ? -13 : 13;\n"
      "        })\n"
      "        .attr(\"text-anchor\", function (d) {\n"
      "          return d.children || d._children ? \"end\" : \"start\";\n"
      "        })\n"
      "        .text(function (d) { return d.data.name; });\n"
      "\n"
      "      // UPDATE\n"
      "      var nodeUpdate = nodeEnter.merge(node);\n"
      "\n"
      "      // Transition to the proper position for the node\n"
      "      nodeUpdate.transition()\n"
      "        .duration(duration)\n"
      "        .attr(\"transform\", function (d) {\n"
      "          return \"translate(\" + d.y + \",\" + d.x + \")\";\n"
      "        });\n"
      "\n"
      "      // Update the node attributes and style\n"
      "      nodeUpdate.select('circle.node')\n"
      "        .attr('r', 10)\n"
      "        .style(\"fill\", function (d) {\n"
      "          return d._children ? \"lightsteelblue\" : \"#fff\";\n"
      "        })\n"
      "        .attr('cursor', 'pointer');\n"
      "\n"
      "\n"
      "      // Remove any exiting nodes\n"
      "      var nodeExit = node.exit().transition()\n"
      "        .duration(duration)\n"
      "        .attr(\"transform\", function (d) {\n"
      "          return \"translate(\" + source.y + \",\" + source.x + \")\";\n"
      "        })\n"
      "        .remove();\n"
      "\n"
      "      // On exit reduce the node circles size to 0\n"
      "      nodeExit.select('circle')\n"
      "        .attr('r', 1e-6);\n"
      "\n"
      "      // On exit reduce the opacity of text labels\n"
      "      nodeExit.select('text')\n"
      "        .style('fill-opacity', 1e-6);\n"
      "\n"
      "      // ****************** links section ***************************\n"
      "\n"
      "      // Update the links...\n"
      "      var link = svg.selectAll('path.link')\n"
      "        .data(links, function (d) { return d.id; });\n"
      "\n"
      "      // Enter any new links at the parent's previous position.\n"
      "      var linkEnter = link.enter().insert('path', \"g\")\n"
      "        .attr(\"class\", \"link\")\n"
      "        .attr('d', function (d) {\n"
      "          var o = { x: source.x0, y: source.y0 }\n"
      "          return diagonal(o, o)\n"
      "        });\n"
      "\n"
      "      // UPDATE\n"
      "      var linkUpdate = linkEnter.merge(link);\n"
      "\n"
      "      // Transition back to the parent element position\n"
      "      linkUpdate.transition()\n"
      "        .duration(duration)\n"
      "        .attr('d', function (d) { return diagonal(d, d.parent) });\n"
      "\n"
      "      // Remove any exiting links\n"
      "      var linkExit = link.exit().transition()\n"
      "        .duration(duration)\n"
      "        .attr('d', function (d) {\n"
      "          var o = { x: source.x, y: source.y }\n"
      "          return diagonal(o, o)\n"
      "        })\n"
      "        .remove();\n"
      "\n"
      "      // Store the old positions for transition.\n"
      "      nodes.forEach(function (d) {\n"
      "        d.x0 = d.x;\n"
      "        d.y0 = d.y;\n"
      "      });\n"
      "\n"
      "      // Creates a curved (diagonal) path from parent to the child "
      "nodes\n"
      "      function diagonal(s, d) {\n"
      "\n"
      "        path = `M ${s.y} ${s.x}\n"
      "            C ${(s.y + d.y) / 2} ${s.x},\n"
      "              ${(s.y + d.y) / 2} ${d.x},\n"
      "              ${d.y} ${d.x}`\n"
      "\n"
      "        return path\n"
      "      }\n"
      "\n"
      "      // Toggle children on click.\n"
      "      function click(d) {\n"
      "        if (d.children) {\n"
      "          d._children = d.children;\n"
      "          d.children = null;\n"
      "        } else {\n"
      "          d.children = d._children;\n"
      "          d._children = null;\n"
      "        }\n"
      "        update(d);\n"
      "      }\n"
      "    }\n"
      "\n"
      "  </script>\n"
      "</body>\n"
      "\n"
      "</html>\n"};
};

}  // namespace kcc
