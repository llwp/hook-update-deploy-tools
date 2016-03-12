<?php
/**
 * @file
 * Drush commands for Hook Deploy Update Tools.
 */

/**
 * Implements hook_drush_command().
 */
function hook_update_deploy_tools_drush_command() {
  $items = array();
  // The 'init' command.
  $items['site-deploy-init'] = array(
    'description' => dt("Creates the site_deploy module"),
    'arguments' => array(
      'directory-path' => dt('The directory path for where to create site_deploy, relative to where hook_deploy_update_tools is installed.'),
    ),
    'options' => array(),
    'examples' => array(
      'drush site-deploy-init' => dt('Creates site_deploy module in !path', array('!path' => '../custom')),
      'drush site-deploy-init features' => dt('Creates site_deploy module in !path', array('!path' => '../features')),
    ),
    'scope' => 'site',
    'aliases' => array('site-deploy-init', 'hook-update-deploy-tools-init'),
  );

  $items['site-deploy-export-rule'] = array(
    'description' => dt("Exports a rule to an export file, saved in location defined in hook_update_deploy_tools config"),
    'arguments' => array(
      'rule-name' => dt('The machine name of the rule to export.  Example: rules_rule_name'),
    ),
    'options' => array(),
    'examples' => array(
      'drush site-deploy-export-rule rules_rule_name' => dt('Creates the export of rule "!path"', array('!path' => 'rules_rule_name')),
    ),
    'scope' => 'site',
    'aliases' => array('site-deploy-export-rule', 'hook-update-deploy-tools-export-rule'),
  );

  $items['site-deploy-export'] = array(
    'description' => dt("Exports an exportable to an export file, saved in location defined in hook_update_deploy_tools config"),
    'arguments' => array(
      'exportable' => dt('The name type of thing to export.  Example: Rules'),
      'rule-name' => dt('The machine name of the exportable item.  Example: rules_rule_name'),
    ),
    'options' => array(),
    'examples' => array(
      'drush site-deploy-export Rules rules_rule_name' => dt('Creates the export of rule "!path"', array('!path' => 'rules_rule_name')),
    ),
    'scope' => 'site',
    'aliases' => array('site-deploy-export', 'hook-update-deploy-tools-export'),
  );

  return $items;
}

/**
 * Implements hook_drush_help().
 */
function hook_update_deploy_tools_drush_help($section) {
  switch ($section) {
    case 'drush:site-deploy-init':
      return dt("Creates a custom site deploy module.");

    // The 'title' meta is used to name a group of commands in `drush help`.
    case 'meta:drush:site-deploy:title':
      return dt("Commands for making life easier for generating a deploy module.");

    // The 'summary' meta item is displayed in `drush help --filter`.
    case 'meta:site-deploy:summary':
      return dt("Assists in making a custom deploy module for this site.");

    case 'drush:site-deploy-export':
      return dt("Exports an exportable to an export file in a location defined by hook_update_deploy_tools config.");

    // The 'title' meta is used to name a group of commands in `drush help`.
    case 'meta:drush:site-deploy-export:title':
      return dt("Command for exporting an exportable to a file.");

    // The 'summary' meta item is displayed in `drush help --filter`.
    case 'meta:site-deploy-export:summary':
      return dt("Exports rules for this site.");
  }
}

/**
 * Implements drush_hook_COMMAND_validate().
 */
function hook_update_deploy_tools_site_deploy_init_validate() {
  if (drush_is_windows()) {
    // $name = drush_get_username();
    // TODO: implement check for elevated process using w32api
    // as sudo is not available for Windows
    // http://php.net/manual/en/book.w32api.php
  }
  else {
    $name = posix_getpwuid(posix_geteuid());
    if ($name['name'] !== 'root') {
      return drush_set_error('HOOK_UPDATE_DEPLOY_TOOLS', dt('There was an error creating your site_deploy module.'));
    }
  }
}

/**
 * Drush command to create site_deploy module.
 *
 * @param string $directory_path
 *   The path and name of the directory  where site_deploy should be created.
 */
function drush_hook_update_deploy_tools_site_deploy_init($directory_path = '../custom') {
  // Check to see if site_deploy already exists in the site.
  $exists = drupal_get_filename('module', 'site_deploy');
  if ($exists) {
    // site_deploy already exists.  Error out.
    $error = dt("The module site_deploy already exists at !location.", array('!location' => $exists));
    drush_log($error, 'error');
  }
  else {
    // Site_deploy does not exist, so proceed with making it.
    $hudt_dir = drupal_get_path('module', 'hook_update_deploy_tools');
    $hudt_dir = drush_normalize_path($hudt_dir);
    $boilerplate_dir = "{$hudt_dir}/boilerplate";
    // Check destination directory exists.
    $destination = "{$hudt_dir}/../{$directory_path}";
    $destination = drush_normalize_path($destination);
    if (is_dir($destination)) {
      // Make the directory site_deploy.
      $made_dir = drush_mkdir("{$destination}/site_deploy", TRUE);
      if ($made_dir) {
        // Move the files.
        drush_copy_dir("{$boilerplate_dir}/info.txt", "{$destination}/site_deploy/site_deploy.info", FILE_EXISTS_ABORT);
        drush_copy_dir("{$boilerplate_dir}/install.php", "{$destination}/site_deploy/site_deploy.install", FILE_EXISTS_ABORT);
        drush_copy_dir("{$boilerplate_dir}/module.php", "{$destination}/site_deploy/site_deploy.module", FILE_EXISTS_ABORT);
        $success = dt("The module 'site_deploy' was created at '!location'", array('!location' => $destination));
        drush_log($success, 'success');
      }

    }
    else {
      $error = dt("The destination of !destination does not seem to exist as a directory.  It should be relative to where hook_update_deploy_tools is found.", array('!destination' => $destination));
      drush_log($error, 'error');
    }
  }
}


/**
 * Drush command to export anything that implements the ExportInterface.
 *
 * @param string $type
 *   The type of thing to export Rules, Menus... must match HUDT class name.
 * @param string $machine_name
 *   The machine name or unique identifier of the item to export.
 */
function drush_hook_update_deploy_tools_site_deploy_export($type, $machine_name) {
  try {
    $msg = array();
    // Check to see if the class exists.
    $class = '\HookUpdateDeployTools\\' . $type;
    \HookUpdateDeployTools\Check::classExists($class);
    // Check that we are dealing with an exportable type (an ExportInterface).
    $implements = class_implements($class);
    if ((is_array($implements)) && in_array('HookUpdateDeployTools\ExportInterface', $implements)) {
      // Check that we have what it needs to export.
      $class::canExport();
      // It can export so can safely proceed.
      $msg = dt('Exported') . ": [$machine_name] => " . $class::export($machine_name);
    }
    else {
      // It does not implement ExportInterface.  Throw an exception.
      $msg = '\HookUpdateDeployTools\@type does not implement ExportInterface so this command can not be called.';
      $vars = array('@type' => $type);
      throw new \HookUpdateDeployTools\HudtException($msg, $vars, WATCHDOG_ERROR, FALSE);
    };
  }
  catch (\Exception $e) {
    // Any errors from this drush command do not need to be watchdog logged.
    $e->logIt = FALSE;
    $vars = array(
      '@error' => (method_exists($e, 'logMessage')) ? $e->logMessage() : $e->getMessage(),
    );

    $msg = dt("site-deploy-export Caught exception:  @error", $vars);
    drush_log($msg, 'error');
    // Clear out the $msg so it is not duplicated in the return.
    $msg = '';
  }

  return $msg;
}

/**
 * Drush command to export a Rule.
 *
 * @deprecated since version 7.x-1.11
 *   Use drush site-deploy-export Rules RULE_MACHINE_NAME instead.
 *   This is just a legacy wrapper now.
 *
 * @param string $rule_name
 *   The machine name of the rule to export.
 */
function drush_hook_update_deploy_tools_site_deploy_export_rule($rule_name = '') {
  return drush_hook_update_deploy_tools_site_deploy_export('Rules', $rule_name);
}