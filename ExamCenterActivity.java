package com.xiaoling.robot.ui.activity;

import android.os.Bundle;
import android.view.View;

import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;

import com.pang.xhttp.request.XRequest;
import com.pang.xhttp.response.XResponse;

import com.xiaoling.robot.R;
import com.xiaoling.robot.api.TestApi;
import com.xiaoling.robot.base.BaseActivity;
import com.xiaoling.robot.http.HttpData;

import butterknife.BindView;

/**
 * @Description: 测评中心-标签页(测评类型)
 * @Package    : com.xiaoling.robot.ui
 * @ClassName  : ExamCenterActivity
 * @Author     : Pang
 * @Version    : V1.0
 * @Time       : 2019/2/27 21:33
 */
public class ExamCenterActivity extends BaseActivity {

    public static final String EXAM_TYPE       = "EXAM_TYPE";
    public static final int EXAM_TYPE_MOOD     = 1; //情绪
    public static final int EXAM_TYPE_MYSELF   = 2; //人际关系
    public static final int EXAM_TYPE_LEARNING = 3;//学习
    public static final int EXAM_TYPE_RELATION = 4;//自我

    @BindView(R.id.btn_motion)   View mMotionBtn;
    @BindView(R.id.btn_self)     View mSelfBtn;
    @BindView(R.id.btn_study)    View mStudyBtn;
    @BindView(R.id.btn_relation) View mRelationBtn;

    public static final int ACTIVITY_EXAMCENTER_ID = 3;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public int getLayoutId() {
        return R.layout.activity_exam_center;
    }

    @Override
    public void initView() {
        // add view initialition code here
        setActionBarType(ACTION_BAR_TYPE_HOME);
        mMotionBtn.setOnClickListener(this);
        mSelfBtn.setOnClickListener(this);
        mStudyBtn.setOnClickListener(this);
        mRelationBtn.setOnClickListener(this);

    }

    @Override
    public void initData() {
        // add data initialition code here

    }

    @Override
    public void onHttpEnd(XRequest<JsonObject> request, XResponse<JsonObject> response) {
        super.onHttpEnd(request, response);

        if (response.getException() != null) {
            toast(R.string.tip_api_error);
            return;
        }
        // log(response.getResult());
        HttpData data = new HttpData(response.getResult());
        if (data.getRes() != 1) {
            toast(data.getMsg(getString(R.string.tip_api_no_msg)));
            return;
        }
        // generate the data from server
        if (request.getUrl().contains(TestApi.URL))//
        {
            JsonArray array = (JsonArray) data.getContent();
            for (JsonElement ele : array) {
                if (ele != null) {
                    JsonObject object = ele.getAsJsonObject();
                  /*  Lines line = new Lines();
                    line.setName(getJsonString(object, "circuit"));
                    line.setId(NumberUtils.getInt(getJsonString(object, "id")));
                    mLinesAdapter.addItem(line);*/
                }

            }
            // data ok, add your code here!

        }// other api return
       /* else if (request.getUrl().contains(AppBindApi.URL))
        {
            toast(data.getMsg(getString(R.string.tip_api_no_msg)));
        }*/
    }


    @Override
    public void onDestroy() {
        super.onDestroy();
    }

    @Override
    public void onResume() {
        super.onResume();
    }
}
