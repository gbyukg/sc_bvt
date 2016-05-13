package com.sugarcrm.sugar.views;

import java.util.HashMap;

import com.sugarcrm.candybean.datasource.FieldSet;
import com.sugarcrm.sugar.VoodooControl;
import com.sugarcrm.sugar.VoodooUtils;
import com.sugarcrm.sugar.WsRestV10;
import com.sugarcrm.sugar.modules.RecordsModule;

/**
 * Models the RecordView for standard SugarCRM modules.
 * @author David Safar <dsafar@sugarcrm.com>
 *
 */
public class BWCEditView extends View {
	public String bottomPaneState = "";
	public HashMap<String, Subpanel> subpanels = new HashMap<String, Subpanel>();
	public ActivityStream activityStream;
	
	public final VoodooControl BTN_SAVE;
	public final VoodooControl BTN_SAVE_CMR;
	public final VoodooControl BTN_SAVE_SC;
	public final VoodooControl BTN_CANCEL_SC;
	public final VoodooControl BTN_CANCEL;
	public final VoodooControl BTN_CANCEL_POPUP;
		
	public final VoodooControl SUB_SAVE_BTN;
	public final VoodooControl SUB_CANCEL_BTN;
	public final VoodooControl SUB_FULLFORM_BTN;
	
	/**
	 * Initializes the EditView and specifies its parent module so that it
	 * knows which fields are available.  
	 * @param	parentModule	the module that owns this RecordView
	 * @throws	Exception
	 */
	public BWCEditView(RecordsModule parentModule) throws Exception {
		super(parentModule);		
		activityStream = new ActivityStream();
		
		// Common control definitions. 
		BTN_SAVE = new VoodooControl("input", "xpath", "//input[@value = 'Save']");
		BTN_SAVE_CMR = new VoodooControl("input", "xpath", "//input[@value = 'Start Request']");
		BTN_SAVE_SC = new VoodooControl("a", "xpath", "//div[@class='btn-toolbar pull-right']/span[not(contains(@class,'hide'))]//a[@class='rowaction btn btn-primary'][@name='save_button']");
		BTN_CANCEL_SC = new VoodooControl("a", "xpath", "//div[@class='btn-toolbar pull-right']//a[@name='cancel_button' and not(contains(@style, 'display: none'))]");
		BTN_CANCEL = new VoodooControl("input", "id", "CANCEL_HEADER");
		addControl("saveButton", "input", "xpath", "//input[@value = 'Save']");
		addControl("cancelButton", "input", "id", "CANCEL_HEADER");
		BTN_CANCEL_POPUP = new VoodooControl("input", "xpath", "//div[@id='dcboxbody']//input[@value='Cancel' and @title='Cancel [Alt+X]']");
				
		SUB_SAVE_BTN = new VoodooControl("input", "xpath", "//form[contains(@id,'form_SubpanelQuickCreate')]//input[contains(@id,'subpanel_save_button')]");
		SUB_CANCEL_BTN = new VoodooControl("input", "xpath", "//form[contains(@id,'form_SubpanelQuickCreate')]//input[contains(@id,'subpanel_cancel_button')]");
		SUB_FULLFORM_BTN = new VoodooControl("input", "xpath", "//form[contains(@id,'form_SubpanelQuickCreate')]//input[contains(@name,'subpanel_full_form_button')]");
	}
	
	/**
	 * Click the Save button.
	 * 
	 * You must already be on the RecordView in edit mode to use this method.
	 * Takes you to the ListView if successful, remains on the RecordView otherwise.
	 * 
	 * @throws Exception
	 */
	public void save() throws Exception {
        if (parentModule.moduleNamePlural == "Tasks"
                || parentModule.moduleNamePlural == "Leads"
                || parentModule.moduleNamePlural == "Opportunities"
                || parentModule.moduleNamePlural == "RevenueLineItems"
                || parentModule.moduleNamePlural == "Notes") {
            saveSC();
        } else {
            VoodooUtils.pause(1000);
            if (parentModule.moduleNamePlural == "Accounts") {
            	BTN_SAVE_CMR.click();
                VoodooUtils.confirmCreateDialog();
            }
            else{
            	BTN_SAVE.click();
            }
            VoodooUtils.closeAlert();
            VoodooUtils.switchBackToWindow();
            VoodooUtils.switchToBWCFrame();
        }
	}
	
	/**
	 * Click the Save button.
	 * 
	 * You must already be on the RecordView in edit mode to use this method.
	 * Takes you to the ListView if successful, remains on the RecordView otherwise.
	 * 
	 * @throws Exception
	 */
	private void saveSC() throws Exception {
		BTN_SAVE_SC.click();
		if (parentModule.moduleNamePlural.equals("Opportunities")){
			VoodooControl opp_alert = new VoodooControl("a","xpath","//div[@class='alert alert-success alert-block']//a[contains(@href,'#Opportunities/')]");
			if (opp_alert.waitForElementVisible(30000)){
				VoodooUtils.current_oppty_name = opp_alert.getText();
				VoodooUtils.log.info("Set current_oppty_name = " + VoodooUtils.current_oppty_name);
			}
		}
		VoodooUtils.waitForAlertExpiration();
	}
	
	/**
	 * Click the Cancel button.
	 * 
	 * You must already be on the RecordView in edit mode to use this method.
	 * Remains on the RecordView, switching it to Detail mode.
	 * 
	 * @throws Exception
	 */
	public void cancel() throws Exception {
        if (parentModule.moduleNamePlural == "Tasks"
                || parentModule.moduleNamePlural == "Leads"
                || parentModule.moduleNamePlural == "Opportunities"
                || parentModule.moduleNamePlural == "RevenueLineItems"
                || parentModule.moduleNamePlural == "Notes") {
			BTN_CANCEL_SC.click();
			VoodooUtils.waitForAlertExpiration();
		}
		else{
			VoodooUtils.switchToBWCFrame();
			BTN_CANCEL.click();
			VoodooUtils.pause(3000);
			VoodooUtils.switchBackToWindow();
		}
	}
	/**
	 * Retrieve a reference to the edit mode version of a field on the RecordView. 
	 * @param fieldName - The VoodooGrimoire name for the desired control.
	 * @return a reference to the control.
	 */
	public VoodooControl getEditField(String fieldName) throws Exception {
		return ((RecordsModule)parentModule).getField(fieldName).editControl;
	}

	public void setFields(FieldSet editedData) throws Exception {
		VoodooUtils.focusFrame("bwc-frame");
		for(String controlName : editedData.keySet()) {
			if (editedData.get(controlName) != null) {
				VoodooUtils.voodoo.log.fine("Editing field: " + controlName);
				String toSet = editedData.get(controlName);
				VoodooUtils.pause(100);
				VoodooUtils.voodoo.log.fine("Setting " + controlName + " field to: "
						+ toSet);
				getEditField(controlName).set(toSet);
				VoodooUtils.pause(100);
			}
		}
		VoodooUtils.focusDefault();
	}
}
