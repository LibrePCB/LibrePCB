var graphConfig = new GitGraph.Template({
  colors: [ "#47E8D4", "#9993FF", "#6BDB52", "#F85BB5", "#FFA657", "#F85BB5" ],
  branch: {
    color: "#000000",
    lineWidth: 3,
    spacingX: 60,
    mergeStyle: "straight",
    showLabel: true,
    labelFont: "normal 10pt Arial",
    labelRotation: 0
  },
  commit: {
    spacingY: -30,
    dot: {
      size: 8,
      strokeColor: "#000000",
      strokeWidth: 4
    },
    tag: {
      font: "normal 10pt Arial",
      color: "yellow"
    },
    message: {
      color: "black",
      font: "normal 11pt Arial",
      displayAuthor: false,
      displayBranch: false,
      displayHash: false,
    }
  },
  arrow: {
    size: 8,
    offset: 3
  }
});

var config = {
  template: graphConfig,
  mode: "extended",
  orientation: "vertical"
};

var featureCol = 0;
var masterCol = 1;
var releaseCol = 2;

var gitgraph = new GitGraph(config);

var master = gitgraph.branch({
  name: "master",
  column: masterCol
});
master.commit("Initial commit");
master.commit("Set FILE_FORMAT_STABLE=0");
master.commit("Set FILE_FORMAT_VERSION='0.1' and APP_VERSION='0.1.0-unstable'");

var feature1 = gitgraph.branch({
  parentBranch: master,
  name: "feature-1",
  column: featureCol
});
feature1.commit({messageDisplay: false});
feature1.commit({messageDisplay: false});
feature1.merge(master, {messageDisplay: false});

var release01 = gitgraph.branch({
  parentBranch: master,
  name: "release/0.1",
  column: releaseCol
});
master.commit("Set FILE_FORMAT_VERSION='0.2' and APP_VERSION='0.2.0-unstable'");
release01.commit("Set FILE_FORMAT_STABLE=1 (freeze file format 0.1!)");
release01.commit("Change resource in .tx/config to 'librepcb-0.1.ts' and update i18n submodule");
release01.commit({
  message: "Set APP_VERSION='0.1.0-rc1'",
  tag: "0.1.0-rc1",
  tagColor: 'gray'
});

var feature2 = gitgraph.branch({
  parentBranch: master,
  name: "feature-2",
  column: featureCol
});
feature2.commit({messageDisplay: false});

release01.commit("Bugfix (cherry-picked from master)");
release01.commit({
  message: "Set APP_VERSION='0.1.0-rc2'",
  tag: "0.1.0-rc2",
  tagColor: 'gray'
});
release01.commit({
  dotStrokeWidth: 8,
  message: "Set APP_VERSION='0.1.0' => OFFICIAL STABLE RELEASE!",
  tag: "0.1.0"
});

feature2.commit({messageDisplay: false});
feature2.commit({messageDisplay: false});
feature2.merge(master, {messageDisplay: false});

release01.commit("New feature (cherry-picked from master)");
release01.commit("Update i18n submodule (if needed)");
release01.commit({
  message: "Set APP_VERSION='0.1.1-rc1'",
  tag: "0.1.1-rc1",
  tagColor: 'gray'
});
release01.commit({
  dotStrokeWidth: 8,
  message: "Set APP_VERSION='0.1.1' => OFFICIAL STABLE RELEASE!",
  tag: "0.1.1"
});

var feature3 = gitgraph.branch({
  parentBranch: master,
  name: "feature-3",
  column: featureCol
});
feature3.commit({messageDisplay: false});
feature3.commit({messageDisplay: false});
feature3.merge(master, {messageDisplay: false});


