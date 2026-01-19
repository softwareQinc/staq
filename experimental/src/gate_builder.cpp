#include "tools_v1/ast/gate_builder.hpp"
#include "tools_v1/ast/expr.hpp"
#include "tools_v1/ast/stmt.hpp"
#include "tools_v1/ast/control_gate.hpp"
#include <stdexcept>

namespace tools_v1::ast {

namespace PrimitiveGate {
    PauliType pauli_from_string(const std::string& str) {
        if (str == "X" || str == "x") return PauliType::X;
        if (str == "Y" || str == "y") return PauliType::Y;
        if (str == "Z" || str == "z") return PauliType::Z;
        if (str == "I" || str == "i") return PauliType::I;
        throw std::invalid_argument("Invalid Pauli string: " + str);
    }
}

// Helper method implementations
template<typename T>
ptr<Gate> GateBuilder<T>::build_pauli_string() {
    if (qubits.size() != paulis.size()) {
        throw std::runtime_error("PauliString requires equal number of qubits and Pauli operators");
    }
    return PauliString::create(pos, std::move(qubits), std::move(paulis));
}

template<typename T>
ptr<Gate> GateBuilder<T>::build_control_gate() {
    if (qubits.size() != 1) {
        throw std::runtime_error("ControlGate requires exactly one control qubit");
    }
    if (!target_gate) {
        throw std::runtime_error("ControlGate requires a target gate");
    }
    return ControlGate::create(pos, std::move(qubits[0]), std::move(target_gate));
}

template<typename T>
ptr<Gate> GateBuilder<T>::build_exp_pauli() {
    if (qubits.size() != paulis.size()) {
        throw std::runtime_error("ExpPauli requires equal number of qubits and Pauli operators");
    }
    if (!angle) {
        throw std::runtime_error("ExpPauli requires an angle expression");
    }
    return ExpPauli::create(pos, std::move(angle), std::move(qubits), std::move(paulis));
}

template<typename T>
GateBuilder<T>& GateBuilder<T>::operator+=(PrimitiveGate::Type gate_type) {
    reset();
    current_type = gate_type;
    return *this;
}

template<typename T>
GateBuilder<T>& GateBuilder<T>::operator,(const VarAccess& qubit) {
    qubits.push_back(qubit);
    return *this;
}

template<typename T>
GateBuilder<T>& GateBuilder<T>::operator,(const std::string& qubit_name) {
    // This is a placeholder - in real implementation, we'd need to parse "q", 1 format
    // For now, we'll assume VarAccess objects are passed directly
    throw std::runtime_error("String qubit parsing not yet implemented");
    return *this;
}

template<typename T>
GateBuilder<T>& GateBuilder<T>::operator,(PauliType pauli) {
    paulis.push_back(pauli);
    return *this;
}

template<typename T>
GateBuilder<T>& GateBuilder<T>::operator,(double angle_value) {
    angle = RealExpr::create(pos, angle_value);
    return *this;
}

template<typename T>
GateBuilder<T>& GateBuilder<T>::operator,(ptr<Expr> expr) {
    angle = std::move(expr);
    return *this;
}

template<typename T>
GateBuilder<T>& GateBuilder<T>::operator,(PrimitiveGate::Type nested_gate_type) {
    // For nested gates like ControlGate, we need to build the target gate first
    // This is a complex case that would require more sophisticated state management
    throw std::runtime_error("Nested gate building not yet implemented");
    return *this;
}

template<typename T>
T GateBuilder<T>::submit() {
    ptr<Gate> built_gate;
    
    switch (current_type) {
        case PrimitiveGate::PAULI_STRING:
            built_gate = build_pauli_string();
            break;
        case PrimitiveGate::CONTROL:
            built_gate = build_control_gate();
            break;
        case PrimitiveGate::EXP_PAULI:
            built_gate = build_exp_pauli();
            break;
        default:
            throw std::runtime_error("Gate type not yet implemented");
    }
    
    reset();
    
    // For single gate builder, return the gate
    if constexpr (std::is_same_v<T, ptr<Gate>>) {
        return built_gate;
    }
    // For vector builder, this will be handled in specialization
    
    return T{};
}

template<typename T>
void GateBuilder<T>::reset() {
    qubits.clear();
    paulis.clear();
    target_gate.reset();
    angle.reset();
}

// GateVectorBuilder specialization
GateVectorBuilder& GateVectorBuilder::operator+=(PrimitiveGate::Type gate_type) {
    GateBuilder<std::vector<ptr<Gate>>>::operator+=(gate_type);
    return *this;
}

GateVectorBuilder& GateVectorBuilder::operator,(PrimitiveGate::Type next_gate_type) {
    // Submit current gate and start new one
    auto gate = GateBuilder<std::vector<ptr<Gate>>>::submit();
    if (!gate.empty()) {
        // Move all gates from the returned vector into our gates vector
        for (auto& g : gate) {
            if (g) {
                gates.push_back(std::move(g));
            }
        }
    }
    return operator+=(next_gate_type);
}

std::vector<ptr<Gate>> GateVectorBuilder::submit() {
    // Submit final gate
    auto gate = GateBuilder<std::vector<ptr<Gate>>>::submit();
    if (!gate.empty()) {
        // Move all gates from the returned vector into our gates vector
        for (auto& g : gate) {
            if (g) {
                gates.push_back(std::move(g));
            }
        }
    }
    return std::move(gates);
}

// Convenience functions
GateVectorBuilder gates() {
    return GateVectorBuilder{};
}

GateBuilder<ptr<Gate>> gate() {
    return GateBuilder<ptr<Gate>>{};
}

// Explicit template instantiation
template class GateBuilder<ptr<Gate>>;
template class GateBuilder<std::vector<ptr<Gate>>>;

} // namespace tools_v1::ast
