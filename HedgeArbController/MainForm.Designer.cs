namespace HedgeArbController
{
    partial class MainForm
    {
        private System.ComponentModel.IContainer components = null;
        private System.Windows.Forms.Label lblCommonPath;
        private System.Windows.Forms.TextBox txtCommonPath;
        private System.Windows.Forms.Label lblSymbol;
        private System.Windows.Forms.TextBox txtSymbol;
        private System.Windows.Forms.GroupBox grpPrice;
        private System.Windows.Forms.Label lblPriceALabel;
        private System.Windows.Forms.Label lblPriceA;
        private System.Windows.Forms.Label lblPriceBLabel;
        private System.Windows.Forms.Label lblPriceB;
        private System.Windows.Forms.Label lblPriceDiffLabel;
        private System.Windows.Forms.Label lblPriceDiff;
        private System.Windows.Forms.GroupBox grpProfit;
        private System.Windows.Forms.Label lblProfitALabel;
        private System.Windows.Forms.Label lblProfitA;
        private System.Windows.Forms.Label lblProfitBLabel;
        private System.Windows.Forms.Label lblProfitB;
        private System.Windows.Forms.Label lblTotalProfitLabel;
        private System.Windows.Forms.Label lblTotalProfit;
        private System.Windows.Forms.GroupBox grpAccounts;
        private System.Windows.Forms.Label lblAccountALabel;
        private System.Windows.Forms.Label lblAccountA;
        private System.Windows.Forms.Label lblBalanceALabel;
        private System.Windows.Forms.Label lblBalanceA;
        private System.Windows.Forms.Label lblEquityALabel;
        private System.Windows.Forms.Label lblEquityA;
        private System.Windows.Forms.Label lblMarginALabel;
        private System.Windows.Forms.Label lblMarginA;
        private System.Windows.Forms.Label lblFreeMarginALabel;
        private System.Windows.Forms.Label lblFreeMarginA;
        private System.Windows.Forms.Label lblLeverageALabel;
        private System.Windows.Forms.Label lblLeverageA;
        private System.Windows.Forms.Label lblBidALabel;
        private System.Windows.Forms.Label lblBidA;
        private System.Windows.Forms.Label lblAskALabel;
        private System.Windows.Forms.Label lblAskA;
        private System.Windows.Forms.Label lblAccountBLabel;
        private System.Windows.Forms.Label lblAccountB;
        private System.Windows.Forms.Label lblBalanceBLabel;
        private System.Windows.Forms.Label lblBalanceB;
        private System.Windows.Forms.Label lblEquityBLabel;
        private System.Windows.Forms.Label lblEquityB;
        private System.Windows.Forms.Label lblMarginBLabel;
        private System.Windows.Forms.Label lblMarginB;
        private System.Windows.Forms.Label lblFreeMarginBLabel;
        private System.Windows.Forms.Label lblFreeMarginB;
        private System.Windows.Forms.Label lblLeverageBLabel;
        private System.Windows.Forms.Label lblLeverageB;
        private System.Windows.Forms.Label lblBidBLabel;
        private System.Windows.Forms.Label lblBidB;
        private System.Windows.Forms.Label lblAskBLabel;
        private System.Windows.Forms.Label lblAskB;
        private System.Windows.Forms.Label lblLastCommandLabel;
        private System.Windows.Forms.Label lblLastCommand;
        private System.Windows.Forms.Label lblAutoCloseInfo;
        private System.Windows.Forms.GroupBox grpBrokerA;
        private System.Windows.Forms.GroupBox grpBrokerB;
        private System.Windows.Forms.TabControl tabControl;
        private System.Windows.Forms.TabPage tabPositions;
        private System.Windows.Forms.TabPage tabHistory;
        private System.Windows.Forms.DataGridView dgvPositionsA;
        private System.Windows.Forms.DataGridView dgvPositionsB;
        private System.Windows.Forms.DataGridView dgvHistoryA;
        private System.Windows.Forms.DataGridView dgvHistoryB;
        private System.Windows.Forms.Label lblPositionsALabel;
        private System.Windows.Forms.Label lblPositionsBLabel;
        private System.Windows.Forms.Label lblHistoryALabel;
        private System.Windows.Forms.Label lblHistoryBLabel;

        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        private void InitializeComponent()
        {
            this.lblCommonPath = new System.Windows.Forms.Label();
            this.txtCommonPath = new System.Windows.Forms.TextBox();
            this.lblSymbol = new System.Windows.Forms.Label();
            this.txtSymbol = new System.Windows.Forms.TextBox();
            this.grpPrice = new System.Windows.Forms.GroupBox();
            this.lblPriceDiff = new System.Windows.Forms.Label();
            this.lblPriceDiffLabel = new System.Windows.Forms.Label();
            this.lblPriceB = new System.Windows.Forms.Label();
            this.lblPriceBLabel = new System.Windows.Forms.Label();
            this.lblPriceA = new System.Windows.Forms.Label();
            this.lblPriceALabel = new System.Windows.Forms.Label();
            this.grpProfit = new System.Windows.Forms.GroupBox();
            this.lblTotalProfit = new System.Windows.Forms.Label();
            this.lblTotalProfitLabel = new System.Windows.Forms.Label();
            this.lblProfitB = new System.Windows.Forms.Label();
            this.lblProfitBLabel = new System.Windows.Forms.Label();
            this.lblProfitA = new System.Windows.Forms.Label();
            this.lblProfitALabel = new System.Windows.Forms.Label();
            this.grpAccounts = new System.Windows.Forms.GroupBox();
            this.grpBrokerB = new System.Windows.Forms.GroupBox();
            this.lblAskB = new System.Windows.Forms.Label();
            this.lblAskBLabel = new System.Windows.Forms.Label();
            this.lblBidB = new System.Windows.Forms.Label();
            this.lblBidBLabel = new System.Windows.Forms.Label();
            this.lblLeverageB = new System.Windows.Forms.Label();
            this.lblLeverageBLabel = new System.Windows.Forms.Label();
            this.lblFreeMarginB = new System.Windows.Forms.Label();
            this.lblFreeMarginBLabel = new System.Windows.Forms.Label();
            this.lblMarginB = new System.Windows.Forms.Label();
            this.lblMarginBLabel = new System.Windows.Forms.Label();
            this.lblEquityB = new System.Windows.Forms.Label();
            this.lblEquityBLabel = new System.Windows.Forms.Label();
            this.lblBalanceB = new System.Windows.Forms.Label();
            this.lblBalanceBLabel = new System.Windows.Forms.Label();
            this.lblAccountB = new System.Windows.Forms.Label();
            this.lblAccountBLabel = new System.Windows.Forms.Label();
            this.grpBrokerA = new System.Windows.Forms.GroupBox();
            this.lblAskA = new System.Windows.Forms.Label();
            this.lblAskALabel = new System.Windows.Forms.Label();
            this.lblBidA = new System.Windows.Forms.Label();
            this.lblBidALabel = new System.Windows.Forms.Label();
            this.lblLeverageA = new System.Windows.Forms.Label();
            this.lblLeverageALabel = new System.Windows.Forms.Label();
            this.lblFreeMarginA = new System.Windows.Forms.Label();
            this.lblFreeMarginALabel = new System.Windows.Forms.Label();
            this.lblMarginA = new System.Windows.Forms.Label();
            this.lblMarginALabel = new System.Windows.Forms.Label();
            this.lblEquityA = new System.Windows.Forms.Label();
            this.lblEquityALabel = new System.Windows.Forms.Label();
            this.lblBalanceA = new System.Windows.Forms.Label();
            this.lblBalanceALabel = new System.Windows.Forms.Label();
            this.lblAccountA = new System.Windows.Forms.Label();
            this.lblAccountALabel = new System.Windows.Forms.Label();
            this.lblLastCommandLabel = new System.Windows.Forms.Label();
            this.lblLastCommand = new System.Windows.Forms.Label();
            this.lblAutoCloseInfo = new System.Windows.Forms.Label();
            this.grpPrice.SuspendLayout();
            this.grpProfit.SuspendLayout();
            this.grpAccounts.SuspendLayout();
            this.grpBrokerB.SuspendLayout();
            this.grpBrokerA.SuspendLayout();
            this.tabControl = new System.Windows.Forms.TabControl();
            this.tabPositions = new System.Windows.Forms.TabPage();
            this.tabHistory = new System.Windows.Forms.TabPage();
            this.dgvPositionsA = new System.Windows.Forms.DataGridView();
            this.dgvPositionsB = new System.Windows.Forms.DataGridView();
            this.dgvHistoryA = new System.Windows.Forms.DataGridView();
            this.dgvHistoryB = new System.Windows.Forms.DataGridView();
            this.lblPositionsALabel = new System.Windows.Forms.Label();
            this.lblPositionsBLabel = new System.Windows.Forms.Label();
            this.lblHistoryALabel = new System.Windows.Forms.Label();
            this.lblHistoryBLabel = new System.Windows.Forms.Label();
            this.tabPositions.SuspendLayout();
            this.tabHistory.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.dgvPositionsA)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.dgvPositionsB)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.dgvHistoryA)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.dgvHistoryB)).BeginInit();
            this.tabControl.SuspendLayout();
            this.SuspendLayout();
            // 
            // lblCommonPath
            // 
            this.lblCommonPath.AutoSize = true;
            this.lblCommonPath.Location = new System.Drawing.Point(12, 15);
            this.lblCommonPath.Name = "lblCommonPath";
            this.lblCommonPath.Size = new System.Drawing.Size(119, 17);
            this.lblCommonPath.TabIndex = 0;
            this.lblCommonPath.Text = "MT公共Files路径:";
            // 
            // txtCommonPath
            // 
            this.txtCommonPath.Location = new System.Drawing.Point(137, 12);
            this.txtCommonPath.Name = "txtCommonPath";
            this.txtCommonPath.Size = new System.Drawing.Size(600, 23);
            this.txtCommonPath.TabIndex = 1;
            this.txtCommonPath.TextChanged += new System.EventHandler(this.txtCommonPath_TextChanged);
            // 
            // lblSymbol
            // 
            this.lblSymbol.AutoSize = true;
            this.lblSymbol.Location = new System.Drawing.Point(12, 45);
            this.lblSymbol.Name = "lblSymbol";
            this.lblSymbol.Size = new System.Drawing.Size(95, 17);
            this.lblSymbol.TabIndex = 2;
            this.lblSymbol.Text = "监控品种(Symbol):";
            // 
            // txtSymbol
            // 
            this.txtSymbol.Location = new System.Drawing.Point(137, 42);
            this.txtSymbol.Name = "txtSymbol";
            this.txtSymbol.Size = new System.Drawing.Size(100, 23);
            this.txtSymbol.TabIndex = 3;
            this.txtSymbol.TextChanged += new System.EventHandler(this.txtSymbol_TextChanged);
            // 
            // grpPrice
            // 
            this.grpPrice.Controls.Add(this.lblPriceDiff);
            this.grpPrice.Controls.Add(this.lblPriceDiffLabel);
            this.grpPrice.Controls.Add(this.lblPriceB);
            this.grpPrice.Controls.Add(this.lblPriceBLabel);
            this.grpPrice.Controls.Add(this.lblPriceA);
            this.grpPrice.Controls.Add(this.lblPriceALabel);
            this.grpPrice.Location = new System.Drawing.Point(12, 75);
            this.grpPrice.Name = "grpPrice";
            this.grpPrice.Size = new System.Drawing.Size(350, 120);
            this.grpPrice.TabIndex = 4;
            this.grpPrice.TabStop = false;
            this.grpPrice.Text = "报价 / 点差";
            // 
            // lblPriceDiff
            // 
            this.lblPriceDiff.AutoSize = true;
            this.lblPriceDiff.Font = new System.Drawing.Font("Microsoft YaHei UI", 10F, System.Drawing.FontStyle.Bold);
            this.lblPriceDiff.Location = new System.Drawing.Point(120, 80);
            this.lblPriceDiff.Name = "lblPriceDiff";
            this.lblPriceDiff.Size = new System.Drawing.Size(15, 19);
            this.lblPriceDiff.TabIndex = 5;
            this.lblPriceDiff.Text = "-";
            // 
            // lblPriceDiffLabel
            // 
            this.lblPriceDiffLabel.AutoSize = true;
            this.lblPriceDiffLabel.Location = new System.Drawing.Point(15, 82);
            this.lblPriceDiffLabel.Name = "lblPriceDiffLabel";
            this.lblPriceDiffLabel.Size = new System.Drawing.Size(99, 17);
            this.lblPriceDiffLabel.TabIndex = 4;
            this.lblPriceDiffLabel.Text = "中间价差(A-B):";
            // 
            // lblPriceB
            // 
            this.lblPriceB.AutoSize = true;
            this.lblPriceB.Location = new System.Drawing.Point(120, 50);
            this.lblPriceB.Name = "lblPriceB";
            this.lblPriceB.Size = new System.Drawing.Size(15, 17);
            this.lblPriceB.TabIndex = 3;
            this.lblPriceB.Text = "-";
            // 
            // lblPriceBLabel
            // 
            this.lblPriceBLabel.AutoSize = true;
            this.lblPriceBLabel.Location = new System.Drawing.Point(15, 50);
            this.lblPriceBLabel.Name = "lblPriceBLabel";
            this.lblPriceBLabel.Size = new System.Drawing.Size(60, 17);
            this.lblPriceBLabel.TabIndex = 2;
            this.lblPriceBLabel.Text = "Broker B:";
            // 
            // lblPriceA
            // 
            this.lblPriceA.AutoSize = true;
            this.lblPriceA.Location = new System.Drawing.Point(120, 25);
            this.lblPriceA.Name = "lblPriceA";
            this.lblPriceA.Size = new System.Drawing.Size(15, 17);
            this.lblPriceA.TabIndex = 1;
            this.lblPriceA.Text = "-";
            // 
            // lblPriceALabel
            // 
            this.lblPriceALabel.AutoSize = true;
            this.lblPriceALabel.Location = new System.Drawing.Point(15, 25);
            this.lblPriceALabel.Name = "lblPriceALabel";
            this.lblPriceALabel.Size = new System.Drawing.Size(60, 17);
            this.lblPriceALabel.TabIndex = 0;
            this.lblPriceALabel.Text = "Broker A:";
            // 
            // grpProfit
            // 
            this.grpProfit.Controls.Add(this.lblTotalProfit);
            this.grpProfit.Controls.Add(this.lblTotalProfitLabel);
            this.grpProfit.Controls.Add(this.lblProfitB);
            this.grpProfit.Controls.Add(this.lblProfitBLabel);
            this.grpProfit.Controls.Add(this.lblProfitA);
            this.grpProfit.Controls.Add(this.lblProfitALabel);
            this.grpProfit.Location = new System.Drawing.Point(380, 75);
            this.grpProfit.Name = "grpProfit";
            this.grpProfit.Size = new System.Drawing.Size(350, 120);
            this.grpProfit.TabIndex = 5;
            this.grpProfit.TabStop = false;
            this.grpProfit.Text = "盈亏（点）";
            // 
            // lblTotalProfit
            // 
            this.lblTotalProfit.AutoSize = true;
            this.lblTotalProfit.Font = new System.Drawing.Font("Microsoft YaHei UI", 10F, System.Drawing.FontStyle.Bold);
            this.lblTotalProfit.ForeColor = System.Drawing.Color.Green;
            this.lblTotalProfit.Location = new System.Drawing.Point(120, 80);
            this.lblTotalProfit.Name = "lblTotalProfit";
            this.lblTotalProfit.Size = new System.Drawing.Size(15, 19);
            this.lblTotalProfit.TabIndex = 5;
            this.lblTotalProfit.Text = "-";
            // 
            // lblTotalProfitLabel
            // 
            this.lblTotalProfitLabel.AutoSize = true;
            this.lblTotalProfitLabel.Location = new System.Drawing.Point(15, 82);
            this.lblTotalProfitLabel.Name = "lblTotalProfitLabel";
            this.lblTotalProfitLabel.Size = new System.Drawing.Size(99, 17);
            this.lblTotalProfitLabel.TabIndex = 4;
            this.lblTotalProfitLabel.Text = "A+B 合计点:";
            // 
            // lblProfitB
            // 
            this.lblProfitB.AutoSize = true;
            this.lblProfitB.Location = new System.Drawing.Point(120, 50);
            this.lblProfitB.Name = "lblProfitB";
            this.lblProfitB.Size = new System.Drawing.Size(15, 17);
            this.lblProfitB.TabIndex = 3;
            this.lblProfitB.Text = "-";
            // 
            // lblProfitBLabel
            // 
            this.lblProfitBLabel.AutoSize = true;
            this.lblProfitBLabel.Location = new System.Drawing.Point(15, 50);
            this.lblProfitBLabel.Name = "lblProfitBLabel";
            this.lblProfitBLabel.Size = new System.Drawing.Size(75, 17);
            this.lblProfitBLabel.TabIndex = 2;
            this.lblProfitBLabel.Text = "Broker B 点:";
            // 
            // lblProfitA
            // 
            this.lblProfitA.AutoSize = true;
            this.lblProfitA.Location = new System.Drawing.Point(120, 25);
            this.lblProfitA.Name = "lblProfitA";
            this.lblProfitA.Size = new System.Drawing.Size(15, 17);
            this.lblProfitA.TabIndex = 1;
            this.lblProfitA.Text = "-";
            // 
            // lblProfitALabel
            // 
            this.lblProfitALabel.AutoSize = true;
            this.lblProfitALabel.Location = new System.Drawing.Point(15, 25);
            this.lblProfitALabel.Name = "lblProfitALabel";
            this.lblProfitALabel.Size = new System.Drawing.Size(75, 17);
            this.lblProfitALabel.TabIndex = 0;
            this.lblProfitALabel.Text = "Broker A 点:";
            // 
            // grpAccounts
            // 
            this.grpAccounts.Controls.Add(this.grpBrokerB);
            this.grpAccounts.Controls.Add(this.grpBrokerA);
            this.grpAccounts.Location = new System.Drawing.Point(12, 210);
            this.grpAccounts.Name = "grpAccounts";
            this.grpAccounts.Size = new System.Drawing.Size(720, 280);
            this.grpAccounts.TabIndex = 6;
            this.grpAccounts.TabStop = false;
            this.grpAccounts.Text = "账户信息";
            // 
            // grpBrokerB
            // 
            this.grpBrokerB.Controls.Add(this.lblAskB);
            this.grpBrokerB.Controls.Add(this.lblAskBLabel);
            this.grpBrokerB.Controls.Add(this.lblBidB);
            this.grpBrokerB.Controls.Add(this.lblBidBLabel);
            this.grpBrokerB.Controls.Add(this.lblLeverageB);
            this.grpBrokerB.Controls.Add(this.lblLeverageBLabel);
            this.grpBrokerB.Controls.Add(this.lblFreeMarginB);
            this.grpBrokerB.Controls.Add(this.lblFreeMarginBLabel);
            this.grpBrokerB.Controls.Add(this.lblMarginB);
            this.grpBrokerB.Controls.Add(this.lblMarginBLabel);
            this.grpBrokerB.Controls.Add(this.lblEquityB);
            this.grpBrokerB.Controls.Add(this.lblEquityBLabel);
            this.grpBrokerB.Controls.Add(this.lblBalanceB);
            this.grpBrokerB.Controls.Add(this.lblBalanceBLabel);
            this.grpBrokerB.Controls.Add(this.lblAccountB);
            this.grpBrokerB.Controls.Add(this.lblAccountBLabel);
            this.grpBrokerB.Location = new System.Drawing.Point(370, 25);
            this.grpBrokerB.Name = "grpBrokerB";
            this.grpBrokerB.Size = new System.Drawing.Size(340, 245);
            this.grpBrokerB.TabIndex = 1;
            this.grpBrokerB.TabStop = false;
            this.grpBrokerB.Text = "Broker B (对冲账户)";
            // 
            // lblAskB
            // 
            this.lblAskB.AutoSize = true;
            this.lblAskB.Location = new System.Drawing.Point(120, 210);
            this.lblAskB.Name = "lblAskB";
            this.lblAskB.Size = new System.Drawing.Size(15, 17);
            this.lblAskB.TabIndex = 15;
            this.lblAskB.Text = "-";
            // 
            // lblAskBLabel
            // 
            this.lblAskBLabel.AutoSize = true;
            this.lblAskBLabel.Location = new System.Drawing.Point(15, 210);
            this.lblAskBLabel.Name = "lblAskBLabel";
            this.lblAskBLabel.Size = new System.Drawing.Size(33, 17);
            this.lblAskBLabel.TabIndex = 14;
            this.lblAskBLabel.Text = "Ask:";
            // 
            // lblBidB
            // 
            this.lblBidB.AutoSize = true;
            this.lblBidB.Location = new System.Drawing.Point(120, 185);
            this.lblBidB.Name = "lblBidB";
            this.lblBidB.Size = new System.Drawing.Size(15, 17);
            this.lblBidB.TabIndex = 13;
            this.lblBidB.Text = "-";
            // 
            // lblBidBLabel
            // 
            this.lblBidBLabel.AutoSize = true;
            this.lblBidBLabel.Location = new System.Drawing.Point(15, 185);
            this.lblBidBLabel.Name = "lblBidBLabel";
            this.lblBidBLabel.Size = new System.Drawing.Size(30, 17);
            this.lblBidBLabel.TabIndex = 12;
            this.lblBidBLabel.Text = "Bid:";
            // 
            // lblLeverageB
            // 
            this.lblLeverageB.AutoSize = true;
            this.lblLeverageB.Location = new System.Drawing.Point(120, 160);
            this.lblLeverageB.Name = "lblLeverageB";
            this.lblLeverageB.Size = new System.Drawing.Size(15, 17);
            this.lblLeverageB.TabIndex = 11;
            this.lblLeverageB.Text = "-";
            // 
            // lblLeverageBLabel
            // 
            this.lblLeverageBLabel.AutoSize = true;
            this.lblLeverageBLabel.Location = new System.Drawing.Point(15, 160);
            this.lblLeverageBLabel.Name = "lblLeverageBLabel";
            this.lblLeverageBLabel.Size = new System.Drawing.Size(44, 17);
            this.lblLeverageBLabel.TabIndex = 10;
            this.lblLeverageBLabel.Text = "杠杆:";
            // 
            // lblFreeMarginB
            // 
            this.lblFreeMarginB.AutoSize = true;
            this.lblFreeMarginB.Location = new System.Drawing.Point(120, 135);
            this.lblFreeMarginB.Name = "lblFreeMarginB";
            this.lblFreeMarginB.Size = new System.Drawing.Size(15, 17);
            this.lblFreeMarginB.TabIndex = 9;
            this.lblFreeMarginB.Text = "-";
            // 
            // lblFreeMarginBLabel
            // 
            this.lblFreeMarginBLabel.AutoSize = true;
            this.lblFreeMarginBLabel.Location = new System.Drawing.Point(15, 135);
            this.lblFreeMarginBLabel.Name = "lblFreeMarginBLabel";
            this.lblFreeMarginBLabel.Size = new System.Drawing.Size(80, 17);
            this.lblFreeMarginBLabel.TabIndex = 8;
            this.lblFreeMarginBLabel.Text = "可用保证金:";
            // 
            // lblMarginB
            // 
            this.lblMarginB.AutoSize = true;
            this.lblMarginB.Location = new System.Drawing.Point(120, 110);
            this.lblMarginB.Name = "lblMarginB";
            this.lblMarginB.Size = new System.Drawing.Size(15, 17);
            this.lblMarginB.TabIndex = 7;
            this.lblMarginB.Text = "-";
            // 
            // lblMarginBLabel
            // 
            this.lblMarginBLabel.AutoSize = true;
            this.lblMarginBLabel.Location = new System.Drawing.Point(15, 110);
            this.lblMarginBLabel.Name = "lblMarginBLabel";
            this.lblMarginBLabel.Size = new System.Drawing.Size(80, 17);
            this.lblMarginBLabel.TabIndex = 6;
            this.lblMarginBLabel.Text = "已用保证金:";
            // 
            // lblEquityB
            // 
            this.lblEquityB.AutoSize = true;
            this.lblEquityB.Location = new System.Drawing.Point(120, 85);
            this.lblEquityB.Name = "lblEquityB";
            this.lblEquityB.Size = new System.Drawing.Size(15, 17);
            this.lblEquityB.TabIndex = 5;
            this.lblEquityB.Text = "-";
            // 
            // lblEquityBLabel
            // 
            this.lblEquityBLabel.AutoSize = true;
            this.lblEquityBLabel.Location = new System.Drawing.Point(15, 85);
            this.lblEquityBLabel.Name = "lblEquityBLabel";
            this.lblEquityBLabel.Size = new System.Drawing.Size(44, 17);
            this.lblEquityBLabel.TabIndex = 4;
            this.lblEquityBLabel.Text = "净值:";
            // 
            // lblBalanceB
            // 
            this.lblBalanceB.AutoSize = true;
            this.lblBalanceB.Location = new System.Drawing.Point(120, 60);
            this.lblBalanceB.Name = "lblBalanceB";
            this.lblBalanceB.Size = new System.Drawing.Size(15, 17);
            this.lblBalanceB.TabIndex = 3;
            this.lblBalanceB.Text = "-";
            // 
            // lblBalanceBLabel
            // 
            this.lblBalanceBLabel.AutoSize = true;
            this.lblBalanceBLabel.Location = new System.Drawing.Point(15, 60);
            this.lblBalanceBLabel.Name = "lblBalanceBLabel";
            this.lblBalanceBLabel.Size = new System.Drawing.Size(44, 17);
            this.lblBalanceBLabel.TabIndex = 2;
            this.lblBalanceBLabel.Text = "余额:";
            // 
            // lblAccountB
            // 
            this.lblAccountB.AutoSize = true;
            this.lblAccountB.Location = new System.Drawing.Point(120, 35);
            this.lblAccountB.Name = "lblAccountB";
            this.lblAccountB.Size = new System.Drawing.Size(15, 17);
            this.lblAccountB.TabIndex = 1;
            this.lblAccountB.Text = "-";
            // 
            // lblAccountBLabel
            // 
            this.lblAccountBLabel.AutoSize = true;
            this.lblAccountBLabel.Location = new System.Drawing.Point(15, 35);
            this.lblAccountBLabel.Name = "lblAccountBLabel";
            this.lblAccountBLabel.Size = new System.Drawing.Size(44, 17);
            this.lblAccountBLabel.TabIndex = 0;
            this.lblAccountBLabel.Text = "账号:";
            // 
            // grpBrokerA
            // 
            this.grpBrokerA.Controls.Add(this.lblAskA);
            this.grpBrokerA.Controls.Add(this.lblAskALabel);
            this.grpBrokerA.Controls.Add(this.lblBidA);
            this.grpBrokerA.Controls.Add(this.lblBidALabel);
            this.grpBrokerA.Controls.Add(this.lblLeverageA);
            this.grpBrokerA.Controls.Add(this.lblLeverageALabel);
            this.grpBrokerA.Controls.Add(this.lblFreeMarginA);
            this.grpBrokerA.Controls.Add(this.lblFreeMarginALabel);
            this.grpBrokerA.Controls.Add(this.lblMarginA);
            this.grpBrokerA.Controls.Add(this.lblMarginALabel);
            this.grpBrokerA.Controls.Add(this.lblEquityA);
            this.grpBrokerA.Controls.Add(this.lblEquityALabel);
            this.grpBrokerA.Controls.Add(this.lblBalanceA);
            this.grpBrokerA.Controls.Add(this.lblBalanceALabel);
            this.grpBrokerA.Controls.Add(this.lblAccountA);
            this.grpBrokerA.Controls.Add(this.lblAccountALabel);
            this.grpBrokerA.Location = new System.Drawing.Point(15, 25);
            this.grpBrokerA.Name = "grpBrokerA";
            this.grpBrokerA.Size = new System.Drawing.Size(340, 245);
            this.grpBrokerA.TabIndex = 0;
            this.grpBrokerA.TabStop = false;
            this.grpBrokerA.Text = "Broker A (主账户)";
            // 
            // lblAskA
            // 
            this.lblAskA.AutoSize = true;
            this.lblAskA.Location = new System.Drawing.Point(120, 210);
            this.lblAskA.Name = "lblAskA";
            this.lblAskA.Size = new System.Drawing.Size(15, 17);
            this.lblAskA.TabIndex = 15;
            this.lblAskA.Text = "-";
            // 
            // lblAskALabel
            // 
            this.lblAskALabel.AutoSize = true;
            this.lblAskALabel.Location = new System.Drawing.Point(15, 210);
            this.lblAskALabel.Name = "lblAskALabel";
            this.lblAskALabel.Size = new System.Drawing.Size(33, 17);
            this.lblAskALabel.TabIndex = 14;
            this.lblAskALabel.Text = "Ask:";
            // 
            // lblBidA
            // 
            this.lblBidA.AutoSize = true;
            this.lblBidA.Location = new System.Drawing.Point(120, 185);
            this.lblBidA.Name = "lblBidA";
            this.lblBidA.Size = new System.Drawing.Size(15, 17);
            this.lblBidA.TabIndex = 13;
            this.lblBidA.Text = "-";
            // 
            // lblBidALabel
            // 
            this.lblBidALabel.AutoSize = true;
            this.lblBidALabel.Location = new System.Drawing.Point(15, 185);
            this.lblBidALabel.Name = "lblBidALabel";
            this.lblBidALabel.Size = new System.Drawing.Size(30, 17);
            this.lblBidALabel.TabIndex = 12;
            this.lblBidALabel.Text = "Bid:";
            // 
            // lblLeverageA
            // 
            this.lblLeverageA.AutoSize = true;
            this.lblLeverageA.Location = new System.Drawing.Point(120, 160);
            this.lblLeverageA.Name = "lblLeverageA";
            this.lblLeverageA.Size = new System.Drawing.Size(15, 17);
            this.lblLeverageA.TabIndex = 11;
            this.lblLeverageA.Text = "-";
            // 
            // lblLeverageALabel
            // 
            this.lblLeverageALabel.AutoSize = true;
            this.lblLeverageALabel.Location = new System.Drawing.Point(15, 160);
            this.lblLeverageALabel.Name = "lblLeverageALabel";
            this.lblLeverageALabel.Size = new System.Drawing.Size(44, 17);
            this.lblLeverageALabel.TabIndex = 10;
            this.lblLeverageALabel.Text = "杠杆:";
            // 
            // lblFreeMarginA
            // 
            this.lblFreeMarginA.AutoSize = true;
            this.lblFreeMarginA.Location = new System.Drawing.Point(120, 135);
            this.lblFreeMarginA.Name = "lblFreeMarginA";
            this.lblFreeMarginA.Size = new System.Drawing.Size(15, 17);
            this.lblFreeMarginA.TabIndex = 9;
            this.lblFreeMarginA.Text = "-";
            // 
            // lblFreeMarginALabel
            // 
            this.lblFreeMarginALabel.AutoSize = true;
            this.lblFreeMarginALabel.Location = new System.Drawing.Point(15, 135);
            this.lblFreeMarginALabel.Name = "lblFreeMarginALabel";
            this.lblFreeMarginALabel.Size = new System.Drawing.Size(80, 17);
            this.lblFreeMarginALabel.TabIndex = 8;
            this.lblFreeMarginALabel.Text = "可用保证金:";
            // 
            // lblMarginA
            // 
            this.lblMarginA.AutoSize = true;
            this.lblMarginA.Location = new System.Drawing.Point(120, 110);
            this.lblMarginA.Name = "lblMarginA";
            this.lblMarginA.Size = new System.Drawing.Size(15, 17);
            this.lblMarginA.TabIndex = 7;
            this.lblMarginA.Text = "-";
            // 
            // lblMarginALabel
            // 
            this.lblMarginALabel.AutoSize = true;
            this.lblMarginALabel.Location = new System.Drawing.Point(15, 110);
            this.lblMarginALabel.Name = "lblMarginALabel";
            this.lblMarginALabel.Size = new System.Drawing.Size(80, 17);
            this.lblMarginALabel.TabIndex = 6;
            this.lblMarginALabel.Text = "已用保证金:";
            // 
            // lblEquityA
            // 
            this.lblEquityA.AutoSize = true;
            this.lblEquityA.Location = new System.Drawing.Point(120, 85);
            this.lblEquityA.Name = "lblEquityA";
            this.lblEquityA.Size = new System.Drawing.Size(15, 17);
            this.lblEquityA.TabIndex = 5;
            this.lblEquityA.Text = "-";
            // 
            // lblEquityALabel
            // 
            this.lblEquityALabel.AutoSize = true;
            this.lblEquityALabel.Location = new System.Drawing.Point(15, 85);
            this.lblEquityALabel.Name = "lblEquityALabel";
            this.lblEquityALabel.Size = new System.Drawing.Size(44, 17);
            this.lblEquityALabel.TabIndex = 4;
            this.lblEquityALabel.Text = "净值:";
            // 
            // lblBalanceA
            // 
            this.lblBalanceA.AutoSize = true;
            this.lblBalanceA.Location = new System.Drawing.Point(120, 60);
            this.lblBalanceA.Name = "lblBalanceA";
            this.lblBalanceA.Size = new System.Drawing.Size(15, 17);
            this.lblBalanceA.TabIndex = 3;
            this.lblBalanceA.Text = "-";
            // 
            // lblBalanceALabel
            // 
            this.lblBalanceALabel.AutoSize = true;
            this.lblBalanceALabel.Location = new System.Drawing.Point(15, 60);
            this.lblBalanceALabel.Name = "lblBalanceALabel";
            this.lblBalanceALabel.Size = new System.Drawing.Size(44, 17);
            this.lblBalanceALabel.TabIndex = 2;
            this.lblBalanceALabel.Text = "余额:";
            // 
            // lblAccountA
            // 
            this.lblAccountA.AutoSize = true;
            this.lblAccountA.Location = new System.Drawing.Point(120, 35);
            this.lblAccountA.Name = "lblAccountA";
            this.lblAccountA.Size = new System.Drawing.Size(15, 17);
            this.lblAccountA.TabIndex = 1;
            this.lblAccountA.Text = "-";
            // 
            // lblAccountALabel
            // 
            this.lblAccountALabel.AutoSize = true;
            this.lblAccountALabel.Location = new System.Drawing.Point(15, 35);
            this.lblAccountALabel.Name = "lblAccountALabel";
            this.lblAccountALabel.Size = new System.Drawing.Size(44, 17);
            this.lblAccountALabel.TabIndex = 0;
            this.lblAccountALabel.Text = "账号:";
            // 
            // lblAutoCloseInfo
            // 
            this.lblAutoCloseInfo.AutoSize = true;
            this.lblAutoCloseInfo.Font = new System.Drawing.Font("Microsoft YaHei UI", 9F, System.Drawing.FontStyle.Bold);
            this.lblAutoCloseInfo.ForeColor = System.Drawing.Color.Green;
            this.lblAutoCloseInfo.Location = new System.Drawing.Point(12, 510);
            this.lblAutoCloseInfo.Name = "lblAutoCloseInfo";
            this.lblAutoCloseInfo.Size = new System.Drawing.Size(200, 17);
            this.lblAutoCloseInfo.TabIndex = 7;
            this.lblAutoCloseInfo.Text = "✓ 自动平仓已启用（盈利达标自动平）";
            // 
            // lblLastCommandLabel
            // 
            this.lblLastCommandLabel.AutoSize = true;
            this.lblLastCommandLabel.Location = new System.Drawing.Point(12, 535);
            this.lblLastCommandLabel.Name = "lblLastCommandLabel";
            this.lblLastCommandLabel.Size = new System.Drawing.Size(80, 17);
            this.lblLastCommandLabel.TabIndex = 8;
            this.lblLastCommandLabel.Text = "最近指令:";
            // 
            // lblLastCommand
            // 
            this.lblLastCommand.AutoSize = true;
            this.lblLastCommand.Font = new System.Drawing.Font("Microsoft YaHei UI", 9F, System.Drawing.FontStyle.Bold);
            this.lblLastCommand.Location = new System.Drawing.Point(98, 535);
            this.lblLastCommand.Name = "lblLastCommand";
            this.lblLastCommand.Size = new System.Drawing.Size(15, 17);
            this.lblLastCommand.TabIndex = 9;
            this.lblLastCommand.Text = "-";
            // 
            // tabControl
            // 
            this.tabControl = new System.Windows.Forms.TabControl();
            this.tabControl.Location = new System.Drawing.Point(12, 560);
            this.tabControl.Size = new System.Drawing.Size(1176, 230);
            this.tabControl.TabIndex = 10;
            // 
            // tabPositions
            // 
            this.tabPositions = new System.Windows.Forms.TabPage();
            this.tabPositions.Text = "当前持仓";
            this.tabPositions.UseVisualStyleBackColor = true;
            // 
            // lblPositionsALabel
            // 
            this.lblPositionsALabel = new System.Windows.Forms.Label();
            this.lblPositionsALabel.Text = "Broker A 持仓:";
            this.lblPositionsALabel.Location = new System.Drawing.Point(10, 10);
            this.lblPositionsALabel.AutoSize = true;
            this.lblPositionsALabel.Font = new System.Drawing.Font("Microsoft YaHei UI", 9F, System.Drawing.FontStyle.Bold);
            // 
            // dgvPositionsA
            // 
            this.dgvPositionsA = new System.Windows.Forms.DataGridView();
            this.dgvPositionsA.Location = new System.Drawing.Point(10, 35);
            this.dgvPositionsA.Size = new System.Drawing.Size(570, 150);
            this.dgvPositionsA.AutoSizeColumnsMode = System.Windows.Forms.DataGridViewAutoSizeColumnsMode.Fill;
            this.dgvPositionsA.ReadOnly = true;
            this.dgvPositionsA.AllowUserToAddRows = false;
            this.dgvPositionsA.RowHeadersVisible = false;
            this.dgvPositionsA.SelectionMode = System.Windows.Forms.DataGridViewSelectionMode.FullRowSelect;
            // 
            // lblPositionsBLabel
            // 
            this.lblPositionsBLabel = new System.Windows.Forms.Label();
            this.lblPositionsBLabel.Text = "Broker B 持仓:";
            this.lblPositionsBLabel.Location = new System.Drawing.Point(590, 10);
            this.lblPositionsBLabel.AutoSize = true;
            this.lblPositionsBLabel.Font = new System.Drawing.Font("Microsoft YaHei UI", 9F, System.Drawing.FontStyle.Bold);
            // 
            // dgvPositionsB
            // 
            this.dgvPositionsB = new System.Windows.Forms.DataGridView();
            this.dgvPositionsB.Location = new System.Drawing.Point(590, 35);
            this.dgvPositionsB.Size = new System.Drawing.Size(570, 150);
            this.dgvPositionsB.AutoSizeColumnsMode = System.Windows.Forms.DataGridViewAutoSizeColumnsMode.Fill;
            this.dgvPositionsB.ReadOnly = true;
            this.dgvPositionsB.AllowUserToAddRows = false;
            this.dgvPositionsB.RowHeadersVisible = false;
            this.dgvPositionsB.SelectionMode = System.Windows.Forms.DataGridViewSelectionMode.FullRowSelect;
            // 
            this.tabPositions.Controls.Add(this.lblPositionsALabel);
            this.tabPositions.Controls.Add(this.dgvPositionsA);
            this.tabPositions.Controls.Add(this.lblPositionsBLabel);
            this.tabPositions.Controls.Add(this.dgvPositionsB);
            // 
            // tabHistory
            // 
            this.tabHistory = new System.Windows.Forms.TabPage();
            this.tabHistory.Text = "交易历史";
            this.tabHistory.UseVisualStyleBackColor = true;
            // 
            // lblHistoryALabel
            // 
            this.lblHistoryALabel = new System.Windows.Forms.Label();
            this.lblHistoryALabel.Text = "Broker A 交易历史:";
            this.lblHistoryALabel.Location = new System.Drawing.Point(10, 10);
            this.lblHistoryALabel.AutoSize = true;
            this.lblHistoryALabel.Font = new System.Drawing.Font("Microsoft YaHei UI", 9F, System.Drawing.FontStyle.Bold);
            // 
            // dgvHistoryA
            // 
            this.dgvHistoryA = new System.Windows.Forms.DataGridView();
            this.dgvHistoryA.Location = new System.Drawing.Point(10, 35);
            this.dgvHistoryA.Size = new System.Drawing.Size(570, 150);
            this.dgvHistoryA.AutoSizeColumnsMode = System.Windows.Forms.DataGridViewAutoSizeColumnsMode.Fill;
            this.dgvHistoryA.ReadOnly = true;
            this.dgvHistoryA.AllowUserToAddRows = false;
            this.dgvHistoryA.RowHeadersVisible = false;
            this.dgvHistoryA.SelectionMode = System.Windows.Forms.DataGridViewSelectionMode.FullRowSelect;
            // 
            // lblHistoryBLabel
            // 
            this.lblHistoryBLabel = new System.Windows.Forms.Label();
            this.lblHistoryBLabel.Text = "Broker B 交易历史:";
            this.lblHistoryBLabel.Location = new System.Drawing.Point(590, 10);
            this.lblHistoryBLabel.AutoSize = true;
            this.lblHistoryBLabel.Font = new System.Drawing.Font("Microsoft YaHei UI", 9F, System.Drawing.FontStyle.Bold);
            // 
            // dgvHistoryB
            // 
            this.dgvHistoryB = new System.Windows.Forms.DataGridView();
            this.dgvHistoryB.Location = new System.Drawing.Point(590, 35);
            this.dgvHistoryB.Size = new System.Drawing.Size(570, 150);
            this.dgvHistoryB.AutoSizeColumnsMode = System.Windows.Forms.DataGridViewAutoSizeColumnsMode.Fill;
            this.dgvHistoryB.ReadOnly = true;
            this.dgvHistoryB.AllowUserToAddRows = false;
            this.dgvHistoryB.RowHeadersVisible = false;
            this.dgvHistoryB.SelectionMode = System.Windows.Forms.DataGridViewSelectionMode.FullRowSelect;
            // 
            this.tabHistory.Controls.Add(this.lblHistoryALabel);
            this.tabHistory.Controls.Add(this.dgvHistoryA);
            this.tabHistory.Controls.Add(this.lblHistoryBLabel);
            this.tabHistory.Controls.Add(this.dgvHistoryB);
            // 
            this.tabControl.TabPages.Add(this.tabPositions);
            this.tabControl.TabPages.Add(this.tabHistory);
            // 
            // MainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 17F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(1200, 800);
            this.Controls.Add(this.lblLastCommand);
            this.Controls.Add(this.lblLastCommandLabel);
            this.Controls.Add(this.lblAutoCloseInfo);
            this.Controls.Add(this.grpAccounts);
            this.Controls.Add(this.grpProfit);
            this.Controls.Add(this.grpPrice);
            this.Controls.Add(this.txtSymbol);
            this.Controls.Add(this.lblSymbol);
            this.Controls.Add(this.txtCommonPath);
            this.Controls.Add(this.lblCommonPath);
            this.Controls.Add(this.tabControl);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.MaximizeBox = false;
            this.Name = "MainForm";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "对冲套利总控界面";
            this.grpPrice.ResumeLayout(false);
            this.grpPrice.PerformLayout();
            this.grpProfit.ResumeLayout(false);
            this.grpProfit.PerformLayout();
            this.grpAccounts.ResumeLayout(false);
            this.grpBrokerB.ResumeLayout(false);
            this.grpBrokerB.PerformLayout();
            this.grpBrokerA.ResumeLayout(false);
            this.grpBrokerA.PerformLayout();
            this.tabPositions.ResumeLayout(false);
            this.tabPositions.PerformLayout();
            this.tabHistory.ResumeLayout(false);
            this.tabHistory.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.dgvPositionsA)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.dgvPositionsB)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.dgvHistoryA)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.dgvHistoryB)).EndInit();
            this.tabControl.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();
        }
    }
}

