/*
 Copyright (C) 2010-2017 Kristian Duske

 This file is part of TrenchBroom.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#include "MapInspector.h"

#include "Model/EntityAttributes.h"
#include "Model/World.h"
#include "View/BorderLine.h"
#include "View/CollapsibleTitledPanel.h"
#include "View/LayerEditor.h"
#include "View/MapDocument.h"
#include "View/ModEditor.h"
#include "View/TitledPanel.h"
#include "View/ViewConstants.h"

#include <kdl/memory_utils.h>

#include <vecmath/vec_io.h>

#include <utility>

#include <QFormLayout>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QLineEdit>

namespace TrenchBroom {
    namespace View {
        // MapInspector

        MapInspector::MapInspector(std::weak_ptr<MapDocument> document, QWidget* parent) :
        TabBookPage(parent) {
            createGui(document);
        }

        void MapInspector::createGui(std::weak_ptr<MapDocument> document) {
            auto* sizer = new QVBoxLayout();
            sizer->setContentsMargins(0, 0, 0, 0);
            sizer->setSpacing(0);

            sizer->addWidget(createLayerEditor(document), 1);
            sizer->addWidget(new BorderLine(BorderLine::Direction::Horizontal), 0);
            sizer->addWidget(createMapProperties(document), 0);
            sizer->addWidget(new BorderLine(BorderLine::Direction::Horizontal), 0);
            sizer->addWidget(createModEditor(document), 0);
            setLayout(sizer);
        }

        QWidget* MapInspector::createLayerEditor(std::weak_ptr<MapDocument> document) {
            TitledPanel* titledPanel = new TitledPanel(tr("Layers"));
            LayerEditor* layerEditor = new LayerEditor(document);

            auto* sizer = new QVBoxLayout();
            sizer->setContentsMargins(0, 0, 0, 0);
            sizer->addWidget(layerEditor, 1);
            titledPanel->getPanel()->setLayout(sizer);

            return titledPanel;
        }

        QWidget* MapInspector::createMapProperties(std::weak_ptr<MapDocument> document) {
            CollapsibleTitledPanel* titledPanel = new CollapsibleTitledPanel(tr("Map Properties"), false);
            auto* editor = new MapPropertiesEditor(document);

            auto* sizer = new QVBoxLayout();
            sizer->setContentsMargins(0, 0, 0, 0);
            sizer->addWidget(editor, 1);
            titledPanel->getPanel()->setLayout(sizer);

            return titledPanel;
        }

        QWidget* MapInspector::createModEditor(std::weak_ptr<MapDocument> document) {
            CollapsibleTitledPanel* titledPanel = new CollapsibleTitledPanel(tr("Mods"), false);
            ModEditor* modEditor = new ModEditor(document);

            auto* sizer = new QVBoxLayout();
            sizer->setContentsMargins(0, 0, 0, 0);
            sizer->addWidget(modEditor, 1);
            titledPanel->getPanel()->setLayout(sizer);

            return titledPanel;
        }

        // MapPropertiesEditor

        MapPropertiesEditor::MapPropertiesEditor(std::weak_ptr<MapDocument> document, QWidget* parent) :
        QWidget(parent),
        m_document(document) {
            createGui();
            bindObservers();
        }

        MapPropertiesEditor::~MapPropertiesEditor() {
            unbindObservers();
        }

        static nonstd::optional<vm::bbox3> parseBounds(const std::string& string) {
            // FIXME: duplicated in GameConfigParser
            if (!vm::can_parse<double, 3u>(string)) {
                return nonstd::nullopt;
            }

            const auto vec = vm::parse<double, 3u>(string);
            return { vm::bbox3(-0.5 * vec, 0.5 * vec) };
        }

        void MapPropertiesEditor::createGui() {
            m_checkBox = new QCheckBox(tr("Map size:"));
            m_sizeBox = new QLineEdit();

            QFormLayout* formLayout = new QFormLayout();
            formLayout->setContentsMargins(0, 0, 0, 0);
            formLayout->setSpacing(0);
            formLayout->addRow(m_checkBox, m_sizeBox);
            setLayout(formLayout);

            connect(m_checkBox, &QAbstractButton::clicked, this, [this](const bool checked) {
                // This signal happens in response to user input only
                auto document = kdl::mem_lock(m_document);
                if (checked) {
                    document->setMapSoftBounds(parseBounds(m_sizeBox->text().toStdString()));                    
                } else {
                    document->setMapSoftBounds(nonstd::nullopt);
                }
            });

            connect(m_sizeBox, &QLineEdit::editingFinished, this, [this]() {
                // This signal happens in response to user input only
                auto document = kdl::mem_lock(m_document);
                if (document) {
                    document->setMapSoftBounds(parseBounds(m_sizeBox->text().toStdString()));
                }
            });

            updateGui();
        }

        void MapPropertiesEditor::bindObservers() {
            auto document = kdl::mem_lock(m_document);
            document->documentWasNewedNotifier.addObserver(this, &MapPropertiesEditor::documentWasNewed);
            document->documentWasLoadedNotifier.addObserver(this, &MapPropertiesEditor::documentWasLoaded);
            document->nodesDidChangeNotifier.addObserver(this, &MapPropertiesEditor::nodesDidChange);
        }

        void MapPropertiesEditor::unbindObservers() {
            if (!kdl::mem_expired(m_document)) {
                auto document = kdl::mem_lock(m_document);
                document->documentWasNewedNotifier.removeObserver(this, &MapPropertiesEditor::documentWasNewed);
                document->documentWasLoadedNotifier.removeObserver(this, &MapPropertiesEditor::documentWasLoaded);
                document->nodesDidChangeNotifier.removeObserver(this, &MapPropertiesEditor::nodesDidChange);
            }
        }

        void MapPropertiesEditor::documentWasNewed(MapDocument*) {
            updateGui();
        }

        void MapPropertiesEditor::documentWasLoaded(MapDocument*) {
            updateGui();
        }

        void MapPropertiesEditor::nodesDidChange(const std::vector<Model::Node*>& nodes) {
            auto document = kdl::mem_lock(m_document);
            if (!document) {
                return;
            }

            for (Model::Node* node : nodes) {
                if (node == document->world()) {
                    updateGui();
                    return;
                }
            }
        }

        /**
         * Refresh the UI from the model
         */
        void MapPropertiesEditor::updateGui() {
            // checkbox is checked iff Model::AttributeNames::SoftMaxMapSize key is set

            auto document = kdl::mem_lock(m_document);
            if (!document || !document->world()) {
                m_sizeBox->setEnabled(false);
                m_checkBox->setChecked(false);
                return;
            }

            Model::World* world = document->world();
            const bool hasBoundsSet = world->hasAttribute(Model::AttributeNames::SoftMaxMapSize);
            const QString boundsQString = QString::fromStdString(world->attribute(Model::AttributeNames::SoftMaxMapSize));

            qDebug() << "MapPropertiesEditor::updateGui: '" << boundsQString << "' (set: " << hasBoundsSet << ")";

            m_checkBox->setChecked(hasBoundsSet);
            m_sizeBox->setEnabled(hasBoundsSet);
            m_sizeBox->setText(boundsQString);            
        }
    }
}
