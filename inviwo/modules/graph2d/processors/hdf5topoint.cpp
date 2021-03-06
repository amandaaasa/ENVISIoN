/*********************************************************************************
 *
 * Copyright (c) 2017 Robert Cranston
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *********************************************************************************/

#include <modules/graph2d/processors/hdf5topoint.h>

#include <inviwo/core/common/inviwo.h>

#include <modules/graph2d/util/stringcomparenatural.h>

namespace inviwo {

const ProcessorInfo HDF5ToPoint::processorInfo_ {
        "org.inviwo.HDF5ToPoint",   // Class identifier.
        "HDF5 To Point",            // Display name.
        "Graphing",                 // Category.
        CodeState::Stable,          // Code state.
        Tags::None,                 // Tags.
    };

const ProcessorInfo HDF5ToPoint::getProcessorInfo() const {
    return processorInfo_;
}

HDF5ToPoint::HDF5ToPoint()
    : Processor()
    , hdf5HandleFlatMultiInport_("hdf5HandleFlatMultiInport")
    , pathSelectionProperty_("pathSelectionProperty", "Path")
    , pathFreezeProperty_("pathFreeze", "Freeze path", false)
    , namePrependParentsProperty_("namePrependParentsProperty", "Prepend parents to name", 0)
    , pointVectorOutport_("pointVectorOutport")
{
    addPort(hdf5HandleFlatMultiInport_);
    hdf5HandleFlatMultiInport_.onChange([this]() {
            if (!pathFreezeProperty_.get())
                pathSelectionOptionsReplace(pathSelectionProperty_);
        });

    addProperty(pathSelectionProperty_);
    addProperty(pathFreezeProperty_);
    addProperty(namePrependParentsProperty_);

    addPort(pointVectorOutport_);
}

void HDF5ToPoint::pathSelectionOptionsReplace(OptionPropertyString& pathSelectionProperty) {

    std::string pathSelectionIdentifierOld;
    if (pathSelectionProperty.size() != 0)
        pathSelectionIdentifierOld = pathSelectionProperty.getSelectedIdentifier();

    std::vector<std::vector<std::string>> pathOptionVectorVector;
    for (const auto& hdf5HandleSharedPtr : hdf5HandleFlatMultiInport_.getVectorData())
    {
        std::vector<std::string> pathOptionVector;
        std::function<void(const H5::Group&, const std::string&)> fill =
            [&fill, &pathOptionVector](const auto& group, const auto& path = "") {
                    for (hsize_t childIndex = 0; childIndex < group.getNumObjs(); ++childIndex) {
                        const auto& childName = group.getObjnameByIdx(childIndex);
                        const auto& childPath = path + "/" + childName;
                        switch (group.getObjTypeByIdx(childIndex)) {
                            case H5G_GROUP: {
                                const auto& childGroup = group.openGroup(childName);
                                fill(childGroup, childPath);
                                break;
                            }
                            case H5G_DATASET: {
                                const auto& childDataSet = group.openDataSet(childName);
                                const auto& childTypeClass = childDataSet.getTypeClass();
                                if (childTypeClass != H5T_FLOAT)
                                    return;
                                const auto& childNdims =
                                    childDataSet.getSpace().getSimpleExtentNdims();
                                if (childNdims != 0)
                                    return;
                                pathOptionVector.push_back(childPath);
                                break;
                            }
                            default:
                                break;
                        }
                    }
                };
        fill(hdf5HandleSharedPtr->getGroup(), "");
        std::sort(pathOptionVector.begin(), pathOptionVector.end(), StringCompareNatural);
        pathOptionVectorVector.push_back(std::move(pathOptionVector));
    }

    std::vector<std::string> pathOptionVector;
    if (pathOptionVectorVector.size() >= 1) {
        pathOptionVector.assign(
                pathOptionVectorVector.front().begin(),
                pathOptionVectorVector.front().end()
            );
        std::vector<std::string> buffer;
        while (pathOptionVectorVector.size() > 1) {
            std::set_intersection(
                    pathOptionVector.begin(),
                    pathOptionVector.end(),
                    pathOptionVectorVector.back().begin(),
                    pathOptionVectorVector.back().end(),
                    std::back_inserter(buffer),
                    StringCompareNatural
                );
            swap(buffer, pathOptionVector);
            buffer.clear();
            pathOptionVectorVector.pop_back();
        }
    }

    std::vector<OptionPropertyStringOption> pathPropertyOptionVector;
    for (const auto& pathOption : pathOptionVector) {
        pathPropertyOptionVector.emplace_back(pathOption, pathOption, pathOption);
    }

    pathSelectionProperty.replaceOptions(pathPropertyOptionVector);

    const auto& identifierVector = pathSelectionProperty.getIdentifiers();
    if (std::any_of(
            identifierVector.begin(),
            identifierVector.end(),
            [&pathSelectionIdentifierOld](const auto& identifier) {
                    return identifier == pathSelectionIdentifierOld;
                }
        )
    ) {
        pathSelectionProperty.setSelectedIdentifier(pathSelectionIdentifierOld);
    }
}

void HDF5ToPoint::process() {

    const auto& pointVectorSharedPtr = std::make_shared<std::vector<Point>>();
    pointVectorOutport_.setData(pointVectorSharedPtr);

    if (pathSelectionProperty_.size() == 0)
        return;

    for (const auto& hdf5HandleSharedPtr : hdf5HandleFlatMultiInport_.getVectorData()) {
        const auto& groupPath = hdf5HandleSharedPtr->getGroup().getObjName();

        const auto& basename = [](const auto& pathString) {
                return pathString.substr(pathString.rfind("/") + 1);
            };
        const auto& dirname = [](const auto& pathString) {
                return pathString.substr(0, pathString.rfind("/"));
            };
        const auto& getAttributeString = [&hdf5HandleSharedPtr](
                const auto& path,
                const auto& name,
                const std::string& fallback = ""
            ) {
                const auto& dataSet = hdf5HandleSharedPtr->getGroup().openDataSet(path);
                if (!dataSet.attrExists(name))
                    return fallback;

                std::string attributeString;
                const auto& attribute = dataSet.openAttribute(name);
                attribute.read(attribute.getDataType(), attributeString);
                return attributeString;
            };

        const auto& path = groupPath + "/" + pathSelectionProperty_.getSelectedValue();

        std::string variableName = "";
        std::string parentPath;
        size_t parentCount;
        for(
            parentPath = groupPath, parentCount = 0;
            !parentPath.empty() && parentCount != namePrependParentsProperty_.get();
            parentPath = dirname(parentPath), ++parentCount
        ) {
            const auto parentBasename = basename(parentPath);
            if (!parentBasename.empty())
                variableName = parentBasename + " " + variableName;
        }
        variableName += getAttributeString(path, "VariableName", basename(path));

        const auto& dataSet = hdf5HandleSharedPtr->getGroup().openDataSet(path);
        float value;
        dataSet.read(&value, dataSet.getDataType());

        pointVectorSharedPtr->emplace_back(Point {
                {
                        variableName,
                        getAttributeString(path, "VariableSymbol"),
                        getAttributeString(path, "QuantityName"),
                        getAttributeString(path, "QuantitySymbol"),
                        getAttributeString(path, "Unit"),
                    },
                value,
            });
    }
}

} // namespace

