#include "graph.h"
#include "op.h"
#include "utils.h"

using namespace LotusIR::Utils;

namespace LotusIR
{
    NodeArg::NodeArg(const std::string& p_name,
        const NodeArgInfo& p_nodeProtoInputOutput)
        : m_name(p_name),
        m_nodeArgTypeAndShape(p_nodeProtoInputOutput)
    {
        if (m_nodeArgTypeAndShape.has_type())
        {
            m_type = OpUtils::ToType(m_nodeArgTypeAndShape.type());
        }
    }

    NodeArg::NodeArg(const std::string& p_name,
        const TypeProto& p_type,
        const TensorShapeProto& p_shape)
        : m_name(p_name)
    {
        *(m_nodeArgTypeAndShape.mutable_type()) = p_type;
        *(m_nodeArgTypeAndShape.mutable_shape()) = p_shape;
        m_type = OpUtils::ToType(p_type);
    }

    const std::string& NodeArg::Name() const
    {
        return m_name;
    }

    const PTYPE& NodeArg::Type() const
    {
        return m_type;
    }

    const TensorShapeProto& NodeArg::Shape() const
    {
        return m_nodeArgTypeAndShape.shape();
    }

    const NodeArgInfo& NodeArg::ToProto() const
    {
        return m_nodeArgTypeAndShape;
    }

    void NodeArg::SetType(PTYPE p_type)
    {
        m_type = p_type;
        *(m_nodeArgTypeAndShape.mutable_type())
            = OpUtils::ToTypeProto(p_type);
    }

    Function::Function(Node* p_node,
        const FunctionDefProto& p_funcProto)
    {
        m_body.reset(new Graph(p_node, p_funcProto));
    }

    Graph* Function::Body()
    {
        return m_body.get();
    }

    const std::string& Function::Name()
    {
        return m_body->Name();
    }

    const FunctionDefProto& Function::ToProto()
    {
        return m_body->ToFuncProto();
    }

    Node::EdgeEnd::EdgeEnd(const Node& p_node, const NodeArg& p_nodeArg)
        : m_node(&p_node),
        m_nodeArg(&p_nodeArg)
    {
    }

    const Node* Node::EdgeEnd::GetNode() const
    {
        return m_node;
    }

    const NodeArg* Node::EdgeEnd::GetNodeArg() const
    {
        return m_nodeArg;
    }

    Node::NodeConstIterator::NodeConstIterator(
        std::set<const Node*>::const_iterator p_iter)
        : m_iter(p_iter)
    {
    }

    bool Node::NodeConstIterator::operator==(
        const NodeConstIterator& p_other) const
    {
        return m_iter == p_other.m_iter;
    }

    bool Node::NodeConstIterator::operator!=(
        const NodeConstIterator& p_other) const
    {
        return m_iter != p_other.m_iter;
    }

    void Node::NodeConstIterator::operator++()
    {
        ++m_iter;
    }

    const Node* Node::NodeConstIterator::operator*()
    {
        return *m_iter;
    }


    Node::Node(const Node& p_other)
    {
        m_name = p_other.m_name;
        m_opType = p_other.m_opType;
        m_inputDefs = p_other.m_inputDefs;
        m_inputs = p_other.m_inputs;
        m_inputNodes = p_other.m_inputNodes;
        m_controlInputs = p_other.m_controlInputs;
        m_outputDefs = p_other.m_outputDefs;
        m_outputNodes = p_other.m_outputNodes;
        m_device = p_other.m_device;
        m_attributes = p_other.m_attributes;
    }

    NODEINDEX Node::Index() const
    {
        return m_index;
    }

    const std::string& Node::Name() const
    {
        return m_name;
    }

    const std::string& Node::OpType() const
    {
        return m_opType;
    }

    const std::string& Node::Description() const
    {
        return m_description;
    }

    const std::vector<NodeArg>& Node::InputDefs() const
    {
        return m_inputDefs;
    }

    std::vector<NodeArg>& Node::Mutable_InputDefs()
    {
        m_graph->m_isGraphValid = false;
        return m_inputDefs;
    }

    Node::NodeConstIterator Node::InputNodes_begin() const
    {
        return NodeConstIterator(m_inputNodes.begin());
    }

    Node::NodeConstIterator Node::InputNodes_end() const
    {
        return NodeConstIterator(m_inputNodes.end());
    }

    Node::NodeConstIterator Node::OutputNodes_begin() const
    {
        return NodeConstIterator(m_outputNodes.begin());
    }

    Node::NodeConstIterator Node::OutputNodes_end() const
    {
        return NodeConstIterator(m_outputNodes.end());
    }

    bool Node::InputEdgeSrcEnd(NodeArg* p_inputArg,
        /*out*/const EdgeEnd** p_inputEdgeSrcEnd)
    {
        if (nullptr == p_inputArg
            || nullptr == p_inputEdgeSrcEnd)
        {
            return false;
        }

        auto edgeEndIter = m_inputs.find(p_inputArg);
        if (m_inputs.end() == edgeEndIter)
        {
            // There's no input edge for the specified input argument.
            return false;
        }

        *p_inputEdgeSrcEnd = &(edgeEndIter->second);
        return true;
    }

    const std::vector<NodeArg>& Node::OutputDefs() const
    {
        return m_outputDefs;
    }

    std::vector<NodeArg>& Node::Mutable_OutputDefs()
    {
        m_graph->m_isGraphValid = false;
        return m_outputDefs;
    }

    const std::string& Node::Device() const
    {
        return m_device;
    }

    void Node::SetDevice(const std::string& p_device)
    {
        m_device = p_device;
    }

    void Node::ToProto(NodeProto& p_proto) const
    {
        // Set name.
        p_proto.set_name(m_name);
        // Set op type.
        p_proto.set_op_type(m_opType);
        // Set doc string.
        p_proto.set_doc_string(m_description);

        // Set control inputs.
        p_proto.clear_control_input();
        for (auto& control_input : m_controlInputs)
        {
            *p_proto.add_control_input() = control_input;
        }

        // Set attributes.
        p_proto.clear_attribute();
        for (auto attribute : m_attributes)
        {
            auto attr = p_proto.add_attribute();
            *attr = attribute.second;
        }

        // Set inputs' defitions.
        p_proto.clear_input();
        for (auto& inputDef : m_inputDefs)
        {
            auto input = p_proto.add_input();
            *input = inputDef.Name();
            auto inputInfo = p_proto.add_input_arg_info();
            *inputInfo = inputDef.ToProto();
            // TODO: add_input_arg_count information. 
        }

        // Set outputs' definition.
        p_proto.clear_output();
        for (auto& outputDef : m_outputDefs)
        {
            auto output = p_proto.add_output();
            *output = outputDef.Name();
            auto outputInfo = p_proto.add_output_arg_info();
            *outputInfo = outputDef.ToProto();
        }
    }

    void Node::Init(const NodeProto& p_nodeProto)
    {
        m_name = p_nodeProto.name();
        m_opType = p_nodeProto.op_type();

        for (size_t i = 0; i < p_nodeProto.input().size(); ++i)
        {
            m_inputDefs.push_back(NodeArg(p_nodeProto.input(i), p_nodeProto.input_arg_info(i)));
        }

        for (size_t i = 0; i < p_nodeProto.output().size(); ++i)
        {
            m_inputDefs.push_back(NodeArg(p_nodeProto.output(i), p_nodeProto.output_arg_info(i)));
        }

        for (auto control_input : p_nodeProto.control_input())
        {
            m_controlInputs.insert(control_input);
        }

        for (int i = 0; i < p_nodeProto.attribute_size(); ++i)
        {
            auto& attr = p_nodeProto.attribute(i);
            m_attributes[attr.name()] = attr;
        }
    }

    void Node::Init(const std::string& p_name,
        const std::string& p_opType,
        const std::string& p_description,
        const std::vector<NodeArg>& p_inputArgs,
        const std::vector<NodeArg>& p_outputArgs)
    {
        m_name = p_name;
        m_opType = p_opType;
        m_description = p_description;
        m_inputDefs = p_inputArgs;
        m_outputDefs = p_outputArgs;
    }

    bool Node::ClearAttribute(const std::string& p_attrName)
    {
        return m_attributes.erase(p_attrName) > 0;
    }

    const NodeAttributes& Node::GetAttributes() const
    {
        return m_attributes;
    }

    bool Graph::NodeIterator::operator==(
        const Graph::NodeIterator& p_other) const
    {
        return (m_graph == p_other.m_graph &&
            m_currentNodeIndex == p_other.m_currentNodeIndex);
    }

    bool Graph::NodeIterator::operator!=(
        const Graph::NodeIterator& p_other) const
    {
        return !(*this == p_other);
    }

    void Graph::NodeIterator::operator++()
    {
        while (true)
        {
            m_currentNodeIndex++;
            if (m_currentNodeIndex >= m_graph->MaxNodeIndex()
                || nullptr != m_graph->GetNode(m_currentNodeIndex))
            {
                return;
            }
        }
    }

    Node* Graph::NodeIterator::operator*()
    {
        return m_graph->GetNode(m_currentNodeIndex);
    }

    Graph::Graph(const GraphProto& p_graphProto)
        : m_graphProto(p_graphProto)
    {
        // This is a main graph.
        m_graphType |= Type::Main;

        // Copy function definitions to a map.
        for (auto funcDef : p_graphProto.function())
        {
            m_funcDefMap[funcDef.name()] = funcDef;
        }

        // Copy initial tensors to a map.
        for (auto tensor : p_graphProto.initializer())
        {
            m_nameToInitialTensor[tensor.name()] = tensor;
        }

        // Add nodes.
        AddSourceSinkNodes();
        for (auto nodeProto : p_graphProto.node())
        {
            AddNode(nodeProto);
        }
    }

    Graph::Graph(Node* p_node,
        const FunctionDefProto& p_functionProto)
    {
        // This is a function (subgraph).
        m_graphType |= Type::Sub;

        m_node = p_node;
        m_funcDefProto = p_functionProto;

        AddSourceSinkNodes();
        for (auto& nodeProto : p_functionProto.node())
        {
            AddNode(nodeProto);
        }
    }

    Graph::Graph(const std::string& p_name,
        GRAPH_VERSION p_irVersion,
        GRAPH_VERSION p_producerVersion,
        const std::string& p_producerTag)
    {
        m_graphProto.set_name(p_name);
        m_graphProto.set_ir_version(p_irVersion);
        m_graphProto.set_producer_version(p_producerVersion);
        m_graphProto.set_producer_tag(p_producerTag);
        m_graphType |= Type::Main;
        AddSourceSinkNodes();
    }

    Status Graph::VerifyNoDuplicateName(
        /*out*/ std::unordered_map<std::string, Node::EdgeEnd>& p_outputArgs)
    {
        p_outputArgs.clear();

        std::set<std::string> nodeNames;
        for (auto nodeIter = Nodes_begin();
            nodeIter != Nodes_end();
            ++nodeIter)
        {
            // Verify node name should be unique.
            std::string nodeName = (*nodeIter)->Name();
            if (nodeNames.end() != nodeNames.find(nodeName))
            {
                // Two nodes with same node name.
                Status status(false,
                    "Error: two nodes with same node name (" + nodeName + ").");
                return status;
            }
            nodeNames.insert(nodeName);

            // Verify node outputs' name should be unique.
            for (auto& outputDef : (*nodeIter)->OutputDefs())
            {
                std::string outputArgname = outputDef.Name();
                if (p_outputArgs.end() != p_outputArgs.find(outputArgname))
                {
                    // Two outputs with same name.
                    Status status(false,
                        "Error: two output args with same name ("
                        + outputArgname + ").");
                    return status;
                }
                p_outputArgs.insert(
                { outputArgname, Node::EdgeEnd(*(*nodeIter), outputDef) });
            }
        }
        return Status::OK();
    }

    void Graph::CleanFunctionDefMap(
        const std::set<std::string>& p_funcDefNames)
    {
        for (auto funcDef : m_funcDefMap)
        {
            if (p_funcDefNames.end() == p_funcDefNames.find(funcDef.first))
            {
                // The <funcDef> is NOT used any more, remove it.
                m_funcDefMap.erase(funcDef.first);
            }
        }
    }

    Status Graph::BuildConnections(
        const std::unordered_map<std::string, Node::EdgeEnd>& p_outputArgs)
    {
        std::unordered_set<Node*> innerNodes;
        for (auto nodeIter = Nodes_begin();
            nodeIter != Nodes_end();
            ++nodeIter)
        {
            if (IsSourceNode((*nodeIter)->Index())
                || IsSinkNode((*nodeIter)->Index()))
            {
                continue;
            }

            auto& inputArgs = (*nodeIter)->InputDefs();
            if (inputArgs.size() > 0)
            {
                // This node needs inputs.

                for (auto& inputArg : inputArgs)
                {
                    auto outputArgIter = p_outputArgs.find(inputArg.Name());
                    if (p_outputArgs.end()
                        == outputArgIter)
                    {
                        // No such outputArg matching this inputArg.
                        // This input arg should be fed when running evaluation.

                        // Add a control edge between <souce> node and this node.
                        AddControlEdge(m_sourceNodeIndex, (*nodeIter)->Index());
                        continue;
                    }

                    // Setup input/output relationship between <*nodeIter>
                    // and <outputArgIter>.
                    (*nodeIter)->m_inputNodes.insert(
                        outputArgIter->second.GetNode());
                    (*nodeIter)->m_inputs.insert({ &inputArg , outputArgIter->second });

                    NODEINDEX outputNodeIndex =
                        outputArgIter->second.GetNode()->Index();
                    m_nodes[outputNodeIndex]->m_outputNodes.insert((*nodeIter));

                    innerNodes.insert(m_nodes[outputNodeIndex].get());
                }
            }
            else
            {
                if ((*nodeIter)->OutputDefs().size() <= 0)
                {
                    // This is a useless node.
                    // It has no input/output.
                    RemoveNode((*nodeIter)->Index());
                }

                // This is a starting node.
                // Add a control edge between <souce> node and this node.
                AddControlEdge(m_sourceNodeIndex, (*nodeIter)->Index());
            }
        }

        for (auto nodeIter = Nodes_begin();
            nodeIter != Nodes_end();
            ++nodeIter)
        {
            if (IsSourceNode((*nodeIter)->Index())
                || IsSinkNode((*nodeIter)->Index()))
            {
                continue;
            }

            if (innerNodes.size() <= 0
                || innerNodes.end() == innerNodes.find((*nodeIter)))
            {
                // This is an ending node.
                // Add a control edge from this node to sink node.
                AddControlEdge((*nodeIter)->Index(), m_sinkNodeIndex);
            }
        }

        return Status::OK();
    }

    Status Graph::CheckIsAcyclic(
        std::vector<NODEINDEX>& p_nodesInToplogicalOrder)
    {
        std::unordered_set<NODEINDEX> visitedNodes;
        std::unordered_set<NODEINDEX> ancestorNodes;
        return DepthFirstAccess(ancestorNodes,
            m_sinkNodeIndex,
            visitedNodes,
            p_nodesInToplogicalOrder);
    }

    Status Graph::DepthFirstAccess(std::unordered_set<NODEINDEX> p_ancestors,
        NODEINDEX p_current,
        std::unordered_set<NODEINDEX>& p_visitedNodes,
        std::vector<NODEINDEX>& p_nodesInToplogicalOrder)
    {
        if (p_visitedNodes.end() != p_visitedNodes.find(p_current))
        {
            // The node has been visited before.
            return Status::OK();
        }

        p_ancestors.insert(p_current);
        for (auto iter = m_nodes[p_current]->InputNodes_begin();
            iter != m_nodes[p_current]->InputNodes_end();
            ++iter)
        {
            if (p_ancestors.end() != p_ancestors.find((*iter)->Index()))
            {
                Status status(false,
                    "Error: the graph is not acyclic.");
                return status;
            }

            RETURN_IF_ERROR(DepthFirstAccess(p_ancestors,
                (*iter)->Index(),
                p_visitedNodes,
                p_nodesInToplogicalOrder));
        }
        p_visitedNodes.insert(p_current);
        p_nodesInToplogicalOrder.push_back(p_current);

        return Status::OK();
    }

    Status Graph::InferAndVerifyTypeMatch(
        const std::vector<NODEINDEX>& p_nodesInToplogicalOrder,
        std::unordered_map<std::string, Node::EdgeEnd>& p_outputArgs,
        /*out*/ std::set<std::string>& p_funcDefNames)
    {
        for (auto nodeIndex : p_nodesInToplogicalOrder)
        {
            if (IsSourceNode(nodeIndex)
                || IsSinkNode(nodeIndex))
            {
                continue;
            }

            auto node = GetNode(nodeIndex);
            std::string nodeName = node->Name();
            std::string op_type = node->OpType();
            const OperatorSchema* op = nullptr;
            bool success
                = OperatorSchemaRegistry::Get()->TryGetOp(op_type, &op);
            if (success)
            {
                // The node refers to a primitive operator.

                // Verify node inputs have same size with operator definition.
                if (op->GetInputs().size() != node->InputDefs().size())
                {
                    // Number of inputs do not match.
                    Status status(false, "Error: node (" + nodeName
                        + ")'s number of inputs do not match its operator ("
                        + op_type + ") specification.");
                    return status;
                }

                // Infer and verify node input arg type information.
                int i = 0;
                std::unordered_map<std::string, PTYPE> typeParameterToTypeMap;
                for (auto& inputDef : node->Mutable_InputDefs())
                {
                    // For each input arg.
                    auto outputArgsIter = p_outputArgs.find(inputDef.Name());
                    if (p_outputArgs.end() == outputArgsIter)
                    {
                        // This input arg should be fed by callers.
                        // This input arg should have type information.
                        if (!inputDef.m_nodeArgTypeAndShape.has_type())
                        {
                            Status status(false,
                                "Node (" + nodeName + ") input arg ("
                                + inputDef.Name()
                                + ") does not have type information.");
                            return status;
                        }
                    }
                    else
                    {
                        // Infer its type by copying from its corresponding
                        // input node's output arg.
                        auto outputArgOfInputNode
                            = outputArgsIter->second.GetNodeArg();

                        inputDef.SetType(outputArgOfInputNode->Type());
                    }

                    // Verify the input arg type complys with operator
                    // definition.
                    auto opFormalParameter = op->GetInputs()[i++];
                    auto iter = opFormalParameter.GetTypes().find(inputDef.Type());
                    if (opFormalParameter.GetTypes().end() == iter)
                    {
                        Status status(false,
                            "Node (" + nodeName + ") input arg ("
                            + inputDef.Name() + ") type does not match operator ("
                            + op->GetName() + ") definition.");
                        return status;
                    }

                    typeParameterToTypeMap[opFormalParameter.GetTypeStr()]
                        = inputDef.Type();
                }

                // Verify node outputs have same size with operator definition.
                if (op->GetOutputs().size() != node->OutputDefs().size())
                {
                    // Number of outputs do not match.
                    Status status(false, "Error: node (" + nodeName
                        + ")'s number of outputs do not match its operator ("
                        + op_type + ") specification.");
                    return status;
                }

                // Infer and verify node output arg type information.
                i = 0;
                for (auto& outputDef : node->Mutable_OutputDefs())
                {
                    // For each output arg.

                    auto opFormalParameter = op->GetOutputs()[i++];

                    // Infer output arg type per input arg type if they share
                    // the same type string. For example, type string is "T" 
                    // for both input arg and output arg.
                    auto inputTypesIter
                        = typeParameterToTypeMap.find(opFormalParameter.GetTypeStr());
                    if (typeParameterToTypeMap.end() != inputTypesIter)
                    {
                        outputDef.SetType(inputTypesIter->second);
                        continue;
                    }

                    if (typeParameterToTypeMap.empty())
                    {
                        // There's no input arg.
                        // The output should be read from an attribute - "CONSTANT_VALUE".
                        auto nodeAttributesIter
                            = node->GetAttributes().find("CONSTANT_VALUE");
                        if (node->GetAttributes().end() == nodeAttributesIter)
                        {
                            Status status(false,
                                "Node (" + nodeName + ") output arg value should"
                                "be specified via node attribute 'CONSTANT_VALUE'.");
                            return status;
                        }

                        AttrType attrType = TypeUtils::GetType(nodeAttributesIter->second);
                        if (AttrType::TENSOR == attrType)
                        {
                            auto& tensor = nodeAttributesIter->second.t();
                            TypeProto typeProto;
                            typeProto.mutable_tensor_type()->set_elem_type(tensor.data_type());
                            outputDef.SetType(OpUtils::ToType(typeProto));
                        }
                        else
                        {
                            Status status(false,
                                "For attribute CONSTANT_VALUE, only Tensor type"
                                "is allowed. The attribute type in this model is "
                                + LotusIR::c_attrTypeStr[(int)attrType] + ".");
                            return status;
                        }

                        continue;
                    }

                    // For case that input arg and output arg have different types.
                    if (outputDef.m_nodeArgTypeAndShape.has_type())
                    {
                        // The output arg has already had type information.
                        // Check whether it matches operator definition.
                        auto iter = opFormalParameter.GetTypes().find(outputDef.Type());
                        if (opFormalParameter.GetTypes().end() == iter)
                        {
                            Status status(false,
                                "Node (" + nodeName + ") output arg ("
                                + outputDef.Name() + ") type does not match operator ("
                                + op->GetName() + ") definition.");
                            return status;
                        }
                        continue;
                    }

                    // Output arg has no type information.
                    if (1 == opFormalParameter.GetTypes().size())
                    {
                        // Infer output arg type as the only one type defined
                        // in operator definition.
                        outputDef.SetType(*(opFormalParameter.GetTypes().begin()));
                        continue;
                    }

                    // Output arg has no type information, and there're
                    // multiple allowed types defined in operator definition.
                    // Type inference fails in this case.
                    Status status(false,
                        "Node (" + nodeName + ") output arg ("
                        + outputDef.Name() + ") type inference failed");
                    return status;
                }

                // Attribute verification and fill node attribute with
                // default value defined in operator definition if needed.
                auto attrParser = op->GetAttributeParser();
                if (nullptr != attrParser)
                {
                    // Attribute parser registered.
                    // Verifying attribute match by running attribute parser.
                    RETURN_IF_ERROR(attrParser(node->GetAttributes()));
                }
                else
                {
                    // No attribute parser registered.
                    auto nodeAttributes = node->GetAttributes();
                    for (auto attrDef : op->GetAttributes())
                    {
                        auto nodeAttrIter = nodeAttributes.find(attrDef.GetName());
                        if (nodeAttributes.end() == nodeAttrIter)
                        {
                            const AttributeProto* defaultValue = nullptr;
                            bool hasDefaultValue
                                = attrDef.HasDefaultValue(&defaultValue);
                            if (!hasDefaultValue)
                            {
                                Status status(false,
                                    "Error: the mandatory attribute ("
                                    + attrDef.GetName() + ") is not specified in Node ("
                                    + nodeName + ").");
                                return status;
                            }

                            node->AddAttribute(attrDef.GetName(), *defaultValue);
                        }
                        else
                        {
                            // Verify node attribute type matching type of
                            // attribute defined in operator definition.
                            auto nodeAttrType = TypeUtils::GetType(nodeAttrIter->second);
                            if (nodeAttrType != attrDef.GetType())
                            {
                                Status status(false,
                                    "Node (" + nodeName + ") attribute ("
                                    + nodeAttrIter->first + ") type does not match operator definition.");
                                return status;
                            }
                        }
                    }
                }
            }
            else
            {
                auto funcIter = m_funcDefMap.find(op_type);
                if (m_funcDefMap.end() == funcIter)
                {
                    // A op_type refers to nothing.
                    Status status(false,
                        "Error: the operator or function (" + op_type
                        + ") refered by node (" + nodeName
                        + ") does not exist.");
                    return status;
                }

                // The node refers to a function.
                p_funcDefNames.insert(op_type);

                // Verify node inputs have same size with function definition.
                if (funcIter->second.input_params_size()
                    != node->InputDefs().size())
                {
                    // Number of inputs do not match.
                    Status status(false, "Error: node (" + nodeName
                        + ")'s number of inputs do not match its function ("
                        + op_type + ") specification.");
                    return status;
                }

                // Verify node outputs have same size with function definition.
                if (funcIter->second.output_params_size()
                    != node->OutputDefs().size())
                {
                    // Number of outputs do not match.
                    Status status(false, "Error: node (" + nodeName
                        + ")'s number of outputs do not match its function ("
                        + op_type + ") specification.");
                    return status;
                }
            }
        }

        return Status::OK();
    }

    Status Graph::Resolve()
    {
        if (m_isGraphValid)
        {
            return Status::OK();
        }

        std::unordered_map<std::string, Node::EdgeEnd> outputArgs;
        RETURN_IF_ERROR(VerifyNoDuplicateName(outputArgs));
        RETURN_IF_ERROR(BuildConnections(outputArgs));

        std::vector<NODEINDEX> nodesInToplogicalOrder;
        RETURN_IF_ERROR(CheckIsAcyclic(nodesInToplogicalOrder));

        std::set<std::string> funcDefNames;
        RETURN_IF_ERROR(InferAndVerifyTypeMatch(nodesInToplogicalOrder,
            outputArgs,
            funcDefNames));
        CleanFunctionDefMap(funcDefNames);

        m_isGraphValid = true;
        return Status::OK();
    }

    void Graph::AddSourceSinkNodes()
    {
        std::vector<NodeArg> emptyArgs;
        m_sourceNodeIndex = AddNode("_Graph_Source",
            "NoOp",
            "Source node internally in a graph.",
            emptyArgs,
            emptyArgs)->Index();
        m_sinkNodeIndex = AddNode("_Graph_Sink",
            "NoOp",
            "Sink node internally in a graph.",
            emptyArgs,
            emptyArgs)->Index();
    }

    GRAPH_VERSION Graph::IrVersion() const
    {
        return m_graphProto.ir_version();
    }

    void Graph::SetIrVersion(GRAPH_VERSION p_irVersion)
    {
        m_graphProto.set_ir_version(p_irVersion);
    }

    GRAPH_VERSION Graph::ProducerVersion() const
    {
        return m_graphProto.producer_version();
    }

    void Graph::SetProducerVersion(GRAPH_VERSION p_producerVersion)
    {
        m_graphProto.set_producer_version(p_producerVersion);
    }

    const std::string& Graph::ProducerTag() const
    {
        return m_graphProto.producer_tag();
    }

    void Graph::SetProducerTag(const std::string& p_producerTag)
    {
        m_graphProto.set_producer_tag(p_producerTag);
    }

    const std::string& Graph::Name() const
    {
        return m_graphProto.name();
    }

    void Graph::SetName(const std::string& p_name)
    {
        m_graphProto.set_name(p_name);
    }

    void Graph::AddInitialTensor(const TensorProto& p_tensor)
    {
        m_nameToInitialTensor[p_tensor.name()] = p_tensor;
    }

    void Graph::RemoveInitialTensor(const std::string& p_tensorName)
    {
        m_nameToInitialTensor.erase(p_tensorName);
    }

    bool Graph::GetInitialTensor(const std::string& p_tensorName,
        TensorProto& p_value) const
    {
        auto iter = m_nameToInitialTensor.find(p_tensorName);
        if (m_nameToInitialTensor.end() == iter)
        {
            return false;
        }
        p_value = iter->second;
        return true;
    }

    bool Graph::AddFunctionDef(const FunctionDefProto& p_funcDef)
    {
        auto funcDefName = p_funcDef.name();
        if (m_funcDefMap.end() != m_funcDefMap.find(funcDefName))
        {
            // Same function definition exists.
            return false;
        }
        m_funcDefMap[funcDefName] = p_funcDef;
        return true;
    }

    void Graph::RemoveFunctionDef(const std::string& p_funcDefName)
    {
        m_funcDefMap.erase(p_funcDefName);
        // Set flag to indicates that the graph needs to be resolved.
        m_isGraphValid = false;
    }

    Node* Graph::GetNode(NODEINDEX p_nodeIndex)
    {
        if (MaxNodeIndex() <= p_nodeIndex)
        {
            return nullptr;
        }

        return m_nodes[p_nodeIndex].get();
    }

    Graph::NodeIterator Graph::Nodes_begin()
    {
        return Graph::NodeIterator(0, this);
    }

    Graph::NodeIterator Graph::Nodes_end()
    {
        return Graph::NodeIterator(MaxNodeIndex(), this);
    }

    NODEINDEX Graph::MaxNodeIndex() const
    {
        return m_nodes.size();
    }

    int Graph::NumberOfNodes()
    {
        return m_numOfNodes;
    }

    Node* Graph::AddNode(const NodeProto& p_nodeProto)
    {
        auto node = AllocateNode();
        node->Init(p_nodeProto);

        // Set flag to indicates that the graph needs to be resolved.
        m_isGraphValid = false;
        return node;
    }

    Node* Graph::AddNode(const std::string& p_name,
        const std::string& p_opType,
        const std::string& p_description,
        const std::vector<NodeArg>& p_inputArgs,
        const std::vector<NodeArg>& p_outputArgs)
    {
        auto node = AllocateNode();
        node->Init(p_name, p_opType, p_description, p_inputArgs, p_outputArgs);
        // Set flag to indicates that the graph needs to be resolved.
        m_isGraphValid = false;
        return node;
    }

    Node* Graph::AddNode(const Node& p_other)
    {
        auto node = AllocateNode();
        *node = p_other;
        // Set flag to indicates that the graph needs to be resolved.
        m_isGraphValid = false;
        return node;
    }

    bool Graph::RemoveNode(NODEINDEX p_index)
    {
        if (MaxNodeIndex() <= p_index || nullptr == m_nodes[p_index])
        {
            return false;
        }

        ReleaseNode(p_index);
        // Set flag to indicates that the graph needs to be resolved.
        m_isGraphValid = false;
        return true;
    }

    bool Graph::AddControlEdge(NODEINDEX p_srcNodeIndex,
        NODEINDEX p_dstNodeIndex)
    {
        if (MaxNodeIndex() <= p_srcNodeIndex
            || MaxNodeIndex() <= p_dstNodeIndex
            || nullptr == m_nodes[p_srcNodeIndex]
            || nullptr == m_nodes[p_dstNodeIndex])
        {
            // Invalid node indexes specified.
            return false;
        }
        m_nodes[p_srcNodeIndex]->
            m_outputNodes.insert(m_nodes[p_dstNodeIndex].get());
        m_nodes[p_dstNodeIndex]->
            m_inputNodes.insert(m_nodes[p_srcNodeIndex].get());
        m_nodes[p_dstNodeIndex]->
            m_controlInputs.insert(m_nodes[p_srcNodeIndex]->Name());

        return true;
    }

    bool Graph::TryGetFunction(NODEINDEX p_index, /*out*/Function** p_function)
    {
        if (MaxNodeIndex() <= p_index || nullptr == p_function)
        {
            return false;
        }

        auto& funcDefName = m_nodes[p_index]->OpType();
        auto funcDefIter = m_funcDefMap.find(funcDefName);
        if (m_funcDefMap.end() == funcDefIter)
        {
            // There's no such function definition.
            return false;
        }

        auto funcIter = m_functionMap.find(funcDefName);
        if (m_functionMap.end() != funcIter)
        {
            // A function instantiation exists.
            *p_function = funcIter->second.get();
            return true;
        }

        m_functionMap[funcDefName] =
            std::unique_ptr<Function>(
                new Function(m_nodes[p_index].get(), funcDefIter->second));

        *p_function = m_functionMap[funcDefName].get();
        return true;
    }

    const GraphProto& Graph::ToGraphProto()
    {
        // Nodes.
        m_graphProto.clear_node();
        for (auto& node : m_nodes)
        {
            if (nullptr == node)
            {
                continue;
            }
            auto nodeProto = m_graphProto.add_node();
            node->ToProto(*nodeProto);
        }

        // Functions.
        m_graphProto.clear_function();
        for (auto& func : m_funcDefMap)
        {
            auto funcDef = m_graphProto.add_function();
            (*funcDef) = func.second;
        }

        // Initial tensors;
        m_graphProto.clear_initializer();
        for (auto item : m_nameToInitialTensor)
        {
            auto tensor = m_graphProto.add_initializer();
            *tensor = item.second;
        }

        return m_graphProto;
    }

    const FunctionDefProto& Graph::ToFuncProto()
    {
        return m_funcDefProto;
    }

    bool Graph::InlineAllFunctions(/*out*/Graph* p_graph) const
    {
        if (nullptr == p_graph)
        {
            return false;
        }

        // TODO: add implementation.
        return true;
    }

    bool Graph::IsSourceNode(NODEINDEX p_index) const
    {
        return m_sourceNodeIndex == p_index;
    }

    bool Graph::IsSinkNode(NODEINDEX p_index) const
    {
        return m_sinkNodeIndex == p_index;
    }

    const Node* Graph::SourceNode() const
    {
        return m_nodes[m_sourceNodeIndex].get();
    }

    const Node* Graph::SinkNode() const
    {
        return m_nodes[m_sinkNodeIndex].get();
    }

    Node* Graph::AllocateNode()
    {
        std::unique_ptr<Node> node(new Node(MaxNodeIndex(), this));
        m_nodes.push_back(std::move(node));
        m_numOfNodes++;
        return m_nodes.back().get();
    }

    void Graph::ReleaseNode(NODEINDEX p_nodeIndex)
    {
        m_nodes[p_nodeIndex] = nullptr;
        m_numOfNodes--;
    }
}