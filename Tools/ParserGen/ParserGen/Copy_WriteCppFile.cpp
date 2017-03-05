#include "ParserGen.h"

void WriteCopyDependenciesImpl(const WString& prefix, ParsingSymbol* visitorType, const CodegenConfig& config, VisitorDependency& dependency, StreamWriter& writer)
{
	writer.WriteLine(L"");
	writer.WriteLine(prefix + L"// CopyFields ----------------------------------------");
	FOREACH(ParsingSymbol*, targetType, dependency.fillDependencies)
	{
		writer.WriteLine(L"");
		writer.WriteString(prefix + L"void " + visitorType->GetName() + L"Visitor::CopyFields(");
		PrintType(targetType, config.classPrefix, writer);
		writer.WriteString(L"* from, ");
		PrintType(targetType, config.classPrefix, writer);
		writer.WriteLine(L"* to)");
		writer.WriteLine(prefix + L"{");

		vint count = targetType->GetSubSymbolCount();
		for (vint i = 0; i < count; i++)
		{
			auto field = targetType->GetSubSymbol(i);
			if (field->GetType() == ParsingSymbol::ClassField)
			{
				auto fieldType = field->GetDescriptorSymbol();
				switch (fieldType->GetType())
				{
				case ParsingSymbol::ClassType:
					writer.WriteLine(prefix + L"\tto->" + field->GetName() + L" = CreateField(from->" + field->GetName() + L");");
					break;
				case ParsingSymbol::ArrayType:
					writer.WriteString(prefix + L"\tFOREACH(vl::Ptr<");
					PrintType(fieldType->GetDescriptorSymbol(), config.classPrefix, writer);
					writer.WriteLine(L">, listItem, from->" + field->GetName() + L")");
					writer.WriteLine(prefix + L"\t{");
					writer.WriteLine(prefix + L"\t\tto->" + field->GetName() + L".Add(CreateField(listItem));");
					writer.WriteLine(prefix + L"\t}");
					break;
				case ParsingSymbol::TokenType:
					writer.WriteLine(prefix + L"\tto->" + field->GetName() + L".codeRange = from->" + field->GetName() + L".codeRange;");
					writer.WriteLine(prefix + L"\tto->" + field->GetName() + L".tokenIndex = from->" + field->GetName() + L".tokenIndex;");
					writer.WriteLine(prefix + L"\tto->" + field->GetName() + L".value = from->" + field->GetName() + L".value;");
					break;
				default:
					writer.WriteLine(prefix + L"\tto->" + field->GetName() + L" = from->" + field->GetName() + L";");
				}
			}
		}

		if (targetType->GetDescriptorSymbol())
		{
			writer.WriteString(prefix + L"\tCopyFields(static_cast<");
			PrintType(targetType->GetDescriptorSymbol(), config.classPrefix, writer);
			writer.WriteString(L"*>(from), static_cast<");
			PrintType(targetType->GetDescriptorSymbol(), config.classPrefix, writer);
			writer.WriteLine(L"*>(to));");
		}
		else
		{
			writer.WriteLine(prefix + L"\tto->codeRange = from->codeRange;");
		}

		writer.WriteLine(prefix + L"}");
	}
	writer.WriteLine(L"");
	writer.WriteLine(prefix + L"// CreateField ---------------------------------------");
	FOREACH(ParsingSymbol*, targetType, dependency.createDependencies)
	{
		writer.WriteLine(L"");
		writer.WriteString(prefix + L"vl::Ptr<");
		PrintType(targetType, config.classPrefix, writer);
		writer.WriteString(L"> " + visitorType->GetName() + L"Visitor::CreateField(vl::Ptr<");
		PrintType(targetType, config.classPrefix, writer);
		writer.WriteLine(L"> from)");
		writer.WriteLine(prefix + L"{");
		writer.WriteLine(prefix + L"\tauto to = vl::MakePtr<" + config.classPrefix + targetType->GetName() + L">();");
		writer.WriteLine(prefix + L"\tCopyFields(from.Obj(), to.Obj());");
		writer.WriteLine(prefix + L"\treturn to;");
		writer.WriteLine(prefix + L"}");
	}
}

void WriteCopyCppFile(const WString& name, const WString& parserCode, Ptr<ParsingDefinition> definition, Ptr<ParsingTable> table, ParsingSymbolManager& manager, const CodegenConfig& config, StreamWriter& writer)
{
	WriteFileComment(name, writer);
	WString prefix = WriteFileBegin(config, L"Copy", writer);
	writer.WriteLine(prefix + L"namespace copy_visitor");
	writer.WriteLine(prefix + L"{");
	prefix += L"\t";

	List<ParsingSymbol*> types;
	EnumerateAllTypes(&manager, manager.GetGlobal(), types);

	FOREACH(ParsingSymbol*, type, types)
	{
		if (type->GetType() == ParsingSymbol::ClassType)
		{
			List<ParsingSymbol*> children;
			SearchChildClasses(type, manager.GetGlobal(), &manager, children);
			if (children.Count() > 0)
			{
				VisitorDependency dependency;
				SortedList<ParsingSymbol*> visitedTypes;

				FOREACH(ParsingSymbol*, subType, children)
				{
					SearchDependencies(subType, &manager, visitedTypes, dependency);
				}

				writer.WriteLine(L"");
				writer.WriteLine(L"/***********************************************************************");
				writer.WriteLine(type->GetName() + L"Visitor");
				writer.WriteLine(L"***********************************************************************/");
				WriteCopyDependenciesImpl(prefix, type, config, dependency, writer);

				writer.WriteLine(L"");
				writer.WriteLine(prefix + L"// Visitor Members -----------------------------------");
				FOREACH(ParsingSymbol*, subType, children)
				{
					writer.WriteLine(L"");
					writer.WriteLine(prefix + L"void " + type->GetName() + L"Visitor::Visit(" + config.classPrefix + subType->GetName() + L"* node)");
					writer.WriteLine(prefix + L"{");
					{
						List<ParsingSymbol*> subChildren;
						SearchChildClasses(subType, manager.GetGlobal(), &manager, subChildren);
						if (subChildren.Count() > 0)
						{
							writer.WriteLine(prefix + L"\tthis->result = Dispatch(node);");
						}
						else
						{
							writer.WriteLine(prefix + L"\tauto newNode = vl::MakePtr<" + config.classPrefix + subType->GetName() + L">();");
							writer.WriteLine(prefix + L"\tCopyFields(node, newNode.Obj());");
							writer.WriteLine(prefix + L"\tthis->result = newNode;");
						}
					}
					writer.WriteLine(prefix + L"}");
				}
			}
		}
	}

	prefix = prefix.Left(prefix.Length() - 1);
	writer.WriteLine(prefix + L"}");
	WriteFileEnd(config, writer);
}